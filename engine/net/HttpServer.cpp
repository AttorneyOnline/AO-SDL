/**
 * @file HttpServer.cpp
 * @brief Event-loop HTTP server implementation.
 *
 * Architecture:
 *   - Poll thread: owns all sockets via platform::Poller, handles accept/read/write
 *   - Worker threads: receive parsed requests via EventChannel, run handlers,
 *     return responses via result channel + notifier to wake the poll thread
 *
 * The poll thread never runs user handler code. Workers never touch sockets.
 * Communication between them uses private EventChannel instances (not the
 * global EventManager singleton) to avoid contention.
 */
#include "net/Http.h"

#include "event/EventChannel.h"
#include "platform/Poll.h"
#include "platform/Socket.h"
#include "utils/Log.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace http {

// ===========================================================================
// Internal types
// ===========================================================================

struct HttpConnection {
    uint64_t id;
    platform::Socket socket;
    std::string recv_buf;
    bool headers_complete = false;
    bool keep_alive = false;
};

// Events passed between poll thread and workers.
// These inherit from Event to satisfy EventChannel's static_assert.

struct WorkItem : public Event {
    uint64_t connection_id;
    Request request;
    Server::Handler handler;
};

struct WorkResult : public Event {
    uint64_t connection_id;
    Response response;
};

// ===========================================================================
// Server implementation
// ===========================================================================

// The actual server state. Server's public methods delegate here.
// This is stored as opaque data inside Server's private members.
// Since Server has many private members already declared in the header
// for API compatibility, we store our event-loop state in a separate struct
// accessed via a static map keyed by Server*.

struct ServerState {
    platform::Poller poller;
    platform::Socket listener;
    int listener_fd = -1;
    int notifier_fd = -1;

    std::unordered_map<uint64_t, HttpConnection> connections;
    uint64_t next_conn_id = 1;

    // Private channels — no global EventManager contention
    EventChannel<WorkItem> work_channel;
    EventChannel<WorkResult> result_channel;

    // Worker pool
    std::vector<std::jthread> workers;
    std::mutex work_mutex;
    std::condition_variable work_cv;

    std::atomic<bool> running{false};

    // Handler tables (copied from Server on listen)
    using HandlerEntry = std::pair<std::string, Server::Handler>;
    std::vector<HandlerEntry> get_handlers;
    std::vector<HandlerEntry> post_handlers;
    std::vector<HandlerEntry> put_handlers;
    std::vector<HandlerEntry> patch_handlers;
    std::vector<HandlerEntry> delete_handlers;
    std::vector<HandlerEntry> options_handlers;

    Headers default_headers;
};

static std::mutex g_state_mutex;
static std::unordered_map<Server*, std::unique_ptr<ServerState>> g_states;

static ServerState& get_state(Server* srv) {
    std::lock_guard lock(g_state_mutex);
    auto& ptr = g_states[srv];
    if (!ptr)
        ptr = std::make_unique<ServerState>();
    return *ptr;
}

static void remove_state(Server* srv) {
    std::lock_guard lock(g_state_mutex);
    g_states.erase(srv);
}

// ===========================================================================
// HTTP parsing helpers
// ===========================================================================

static bool find_header_end(const std::string& buf) {
    return buf.find("\r\n\r\n") != std::string::npos;
}

static Server::Handler find_handler(const std::vector<ServerState::HandlerEntry>& handlers, const std::string& path,
                                    Request& req) {
    for (auto& [pattern, handler] : handlers) {
        // Simple prefix/exact match. PathParamsMatcher handles :params.
        if (pattern == path)
            return handler;

        // Try path params pattern (e.g. "/api/users/:id")
        detail::PathParamsMatcher matcher(pattern);
        if (matcher.match(req))
            return handler;
    }
    return nullptr;
}

static Request parse_http_request(const std::string& raw_headers, const std::string& body) {
    Request req;

    auto header_end = raw_headers.find("\r\n\r\n");
    std::string headers_str = raw_headers.substr(0, header_end);

    // Parse request line
    auto first_nl = headers_str.find("\r\n");
    std::string req_line = headers_str.substr(0, first_nl);

    auto sp1 = req_line.find(' ');
    auto sp2 = req_line.find(' ', sp1 + 1);
    if (sp1 != std::string::npos && sp2 != std::string::npos) {
        req.method = req_line.substr(0, sp1);
        req.path = req_line.substr(sp1 + 1, sp2 - sp1 - 1);
        req.version = req_line.substr(sp2 + 1);
    }

    // Parse query params from path
    auto qmark = req.path.find('?');
    if (qmark != std::string::npos) {
        std::string query = req.path.substr(qmark + 1);
        req.path = req.path.substr(0, qmark);

        size_t pos = 0;
        while (pos < query.size()) {
            auto amp = query.find('&', pos);
            if (amp == std::string::npos)
                amp = query.size();
            auto eq = query.find('=', pos);
            if (eq < amp) {
                req.params.emplace(query.substr(pos, eq - pos), query.substr(eq + 1, amp - eq - 1));
            }
            pos = amp + 1;
        }
    }

    // Parse headers
    size_t pos = first_nl + 2;
    while (pos < headers_str.size()) {
        auto line_end = headers_str.find("\r\n", pos);
        if (line_end == std::string::npos)
            line_end = headers_str.size();
        std::string line = headers_str.substr(pos, line_end - pos);
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string val = line.substr(colon + 1);
            while (!val.empty() && val[0] == ' ')
                val.erase(val.begin());
            req.headers.emplace(std::move(key), std::move(val));
        }
        pos = line_end + 2;
    }

    req.body = body;
    req.target = req.path;

    return req;
}

static std::string serialize_response(const Response& res, const Headers& default_headers) {
    std::string out;
    out.reserve(256 + res.body.size());
    out += "HTTP/1.1 " + std::to_string(res.status) + " " + status_message(res.status) + "\r\n";

    // Default headers first
    for (auto& [k, v] : default_headers) {
        out += k + ": " + v + "\r\n";
    }

    // Response headers (may override defaults)
    for (auto& [k, v] : res.headers) {
        out += k + ": " + v + "\r\n";
    }

    if (!res.body.empty()) {
        out += "Content-Length: " + std::to_string(res.body.size()) + "\r\n";
    }

    out += "\r\n";
    out += res.body;
    return out;
}

// ===========================================================================
// Worker thread loop
// ===========================================================================

static void worker_loop(ServerState& state, std::stop_token st) {
    while (!st.stop_requested()) {
        // Wait for work
        {
            std::unique_lock lock(state.work_mutex);
            state.work_cv.wait(lock, [&] { return state.work_channel.has_events() || st.stop_requested(); });
        }

        if (st.stop_requested())
            break;

        auto item = state.work_channel.get_event();
        if (!item)
            continue;

        Response res;
        res.status = 200;

        try {
            if (item->handler) {
                item->handler(item->request, res);
            }
            else {
                res.status = 404;
                res.set_content("Not Found", "text/plain");
            }
        }
        catch (const std::exception& e) {
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
            Log::log_print(ERR, "HTTP handler exception: %s", e.what());
        }

        WorkResult result;
        result.connection_id = item->connection_id;
        result.response = std::move(res);
        state.result_channel.publish(std::move(result));

        // Wake the poll thread
        state.poller.notify();
    }
}

// ===========================================================================
// Poll loop (runs on the thread that called listen_after_bind)
// ===========================================================================

static void poll_loop(Server* srv, ServerState& state) {
    constexpr int MAX_EVENTS = 128;
    platform::Poller::Event events[MAX_EVENTS];

    while (state.running.load()) {
        int n = state.poller.poll(events, MAX_EVENTS, 100);

        for (int i = 0; i < n; ++i) {
            auto& ev = events[i];

            if (ev.fd == state.listener_fd) {
                // Accept new connections
                while (true) {
                    std::string remote_addr;
                    uint16_t remote_port = 0;
                    auto client = platform::tcp_accept(state.listener, remote_addr, remote_port);
                    if (!client.valid())
                        break;

                    client.set_non_blocking(true);
                    int cfd = client.fd();

                    HttpConnection conn;
                    conn.id = state.next_conn_id++;
                    conn.socket = std::move(client);

                    state.poller.add(cfd, platform::Poller::Readable);
                    state.connections.emplace(conn.id, std::move(conn));
                }
            }
            else if (ev.fd == state.notifier_fd) {
                state.poller.drain_notifier();

                // Process completed results
                while (auto result = state.result_channel.get_event()) {
                    // Find the connection
                    HttpConnection* conn = nullptr;
                    for (auto& [id, c] : state.connections) {
                        if (id == result->connection_id) {
                            conn = &c;
                            break;
                        }
                    }
                    if (!conn)
                        continue;

                    std::string response_data = serialize_response(result->response, state.default_headers);

                    // Switch to blocking for the write — non-blocking send
                    // can't flush large responses in one go.
                    conn->socket.set_non_blocking(false);
                    size_t total = 0;
                    while (total < response_data.size()) {
                        ssize_t w = conn->socket.send(response_data.data() + total, response_data.size() - total);
                        if (w <= 0)
                            break;
                        total += static_cast<size_t>(w);
                    }
                    conn->socket.set_non_blocking(true);

                    // Close connection (no keep-alive for now)
                    state.poller.remove(conn->socket.fd());
                    state.connections.erase(result->connection_id);
                }
            }
            else {
                // Client socket readable — find connection by fd
                HttpConnection* conn = nullptr;
                for (auto& [id, c] : state.connections) {
                    if (c.socket.fd() == ev.fd) {
                        conn = &c;
                        break;
                    }
                }
                if (!conn)
                    continue;

                // Read data
                char buf[8192];
                ssize_t r = conn->socket.recv(buf, sizeof(buf));
                if (r <= 0) {
                    // Connection closed or error
                    state.poller.remove(ev.fd);
                    state.connections.erase(conn->id);
                    continue;
                }
                conn->recv_buf.append(buf, static_cast<size_t>(r));

                // Check if headers are complete
                if (!conn->headers_complete && find_header_end(conn->recv_buf)) {
                    conn->headers_complete = true;

                    auto header_end = conn->recv_buf.find("\r\n\r\n");
                    std::string headers_str = conn->recv_buf.substr(0, header_end + 4);
                    std::string body = conn->recv_buf.substr(header_end + 4);

                    Request req = parse_http_request(headers_str, body);

                    // Find handler
                    Server::Handler handler;
                    if (req.method == "GET")
                        handler = find_handler(state.get_handlers, req.path, req);
                    else if (req.method == "POST")
                        handler = find_handler(state.post_handlers, req.path, req);
                    else if (req.method == "PUT")
                        handler = find_handler(state.put_handlers, req.path, req);
                    else if (req.method == "PATCH")
                        handler = find_handler(state.patch_handlers, req.path, req);
                    else if (req.method == "DELETE")
                        handler = find_handler(state.delete_handlers, req.path, req);
                    else if (req.method == "OPTIONS")
                        handler = find_handler(state.options_handlers, req.path, req);

                    WorkItem item;
                    item.connection_id = conn->id;
                    item.request = std::move(req);
                    item.handler = std::move(handler);

                    state.work_channel.publish(std::move(item));
                    state.work_cv.notify_one();
                }
            }
        }
    }
}

// ===========================================================================
// Server public methods
// ===========================================================================

Server::Server() {
    get_state(this); // ensure state exists
    new_task_queue = [] { return static_cast<TaskQueue*>(new ThreadPool(CPPHTTPLIB_THREAD_POOL_COUNT)); };
}

Server::~Server() {
    stop();
    remove_state(this);
}

bool Server::is_valid() const {
    return true;
}

// Route registration — store handler with pattern string
Server& Server::Get(const std::string& pattern, Handler handler) {
    get_state(this).get_handlers.emplace_back(pattern, std::move(handler));
    return *this;
}

Server& Server::Post(const std::string& pattern, Handler handler) {
    get_state(this).post_handlers.emplace_back(pattern, std::move(handler));
    return *this;
}

Server& Server::Post(const std::string&, HandlerWithContentReader) {
    return *this;
}

Server& Server::Put(const std::string& pattern, Handler handler) {
    get_state(this).put_handlers.emplace_back(pattern, std::move(handler));
    return *this;
}

Server& Server::Put(const std::string&, HandlerWithContentReader) {
    return *this;
}

Server& Server::Patch(const std::string& pattern, Handler handler) {
    get_state(this).patch_handlers.emplace_back(pattern, std::move(handler));
    return *this;
}

Server& Server::Patch(const std::string&, HandlerWithContentReader) {
    return *this;
}

Server& Server::Delete(const std::string& pattern, Handler handler) {
    get_state(this).delete_handlers.emplace_back(pattern, std::move(handler));
    return *this;
}

Server& Server::Delete(const std::string&, HandlerWithContentReader) {
    return *this;
}

Server& Server::Options(const std::string& pattern, Handler handler) {
    get_state(this).options_handlers.emplace_back(pattern, std::move(handler));
    return *this;
}

// Configuration
Server& Server::set_default_headers(Headers headers) {
    get_state(this).default_headers = std::move(headers);
    default_headers_ = get_state(this).default_headers;
    return *this;
}

bool Server::bind_to_port(const std::string& host, int port, int) {
    auto& state = get_state(this);
    try {
        state.listener = platform::tcp_listen(host, static_cast<uint16_t>(port));
        state.listener.set_non_blocking(true);
        state.listener_fd = state.listener.fd();
        state.poller.add(state.listener_fd, platform::Poller::Readable);
        state.notifier_fd = state.poller.create_notifier();
        return true;
    }
    catch (const std::exception& e) {
        Log::log_print(ERR, "HTTP bind failed: %s", e.what());
        return false;
    }
}

int Server::bind_to_any_port(const std::string& host, int socket_flags) {
    if (!bind_to_port(host, 0, socket_flags))
        return -1;

    auto& state = get_state(this);
    uint16_t port = state.listener.local_port();
    return (port > 0) ? static_cast<int>(port) : -1;
}

bool Server::listen_after_bind() {
    auto& state = get_state(this);
    state.running.store(true);
    is_running_.store(true);

    // Start worker threads
    size_t num_workers = CPPHTTPLIB_THREAD_POOL_COUNT;
    for (size_t i = 0; i < num_workers; ++i) {
        state.workers.emplace_back([&state](std::stop_token st) { worker_loop(state, st); });
    }

    // Run poll loop on this thread (blocks)
    poll_loop(this, state);

    // Cleanup workers
    for (auto& w : state.workers)
        w.request_stop();
    state.work_cv.notify_all();
    state.workers.clear();

    is_running_.store(false);
    return true;
}

bool Server::listen(const std::string& host, int port, int socket_flags) {
    if (!bind_to_port(host, port, socket_flags))
        return false;
    return listen_after_bind();
}

bool Server::is_running() const {
    return is_running_.load();
}
void Server::wait_until_ready() const { /* poll loop starts immediately */
}

void Server::stop() {
    auto& state = get_state(this);
    state.running.store(false);
    is_running_.store(false);
    state.poller.notify(); // wake poll loop
}

void Server::decommission() {
    stop();
}

// Stubs for methods not yet needed
bool Server::set_base_dir(const std::string&, const std::string&) {
    return false;
}
bool Server::set_mount_point(const std::string&, const std::string&, Headers) {
    return false;
}
bool Server::remove_mount_point(const std::string&) {
    return false;
}
Server& Server::set_file_extension_and_mimetype_mapping(const std::string&, const std::string&) {
    return *this;
}
Server& Server::set_default_file_mimetype(const std::string&) {
    return *this;
}
Server& Server::set_file_request_handler(Handler) {
    return *this;
}
Server& Server::set_error_handler_core(HandlerWithResponse, std::true_type) {
    return *this;
}
Server& Server::set_error_handler_core(Handler, std::false_type) {
    return *this;
}
Server& Server::set_exception_handler(ExceptionHandler) {
    return *this;
}
Server& Server::set_pre_routing_handler(HandlerWithResponse) {
    return *this;
}
Server& Server::set_post_routing_handler(Handler) {
    return *this;
}
Server& Server::set_expect_100_continue_handler(Expect100ContinueHandler) {
    return *this;
}
Server& Server::set_logger(Logger) {
    return *this;
}
Server& Server::set_address_family(int) {
    return *this;
}
Server& Server::set_tcp_nodelay(bool) {
    return *this;
}
Server& Server::set_ipv6_v6only(bool) {
    return *this;
}
Server& Server::set_socket_options(SocketOptions) {
    return *this;
}
Server& Server::set_header_writer(std::function<ssize_t(Stream&, Headers&)> const&) {
    return *this;
}
Server& Server::set_keep_alive_max_count(size_t) {
    return *this;
}
Server& Server::set_keep_alive_timeout(time_t) {
    return *this;
}
Server& Server::set_read_timeout(time_t, time_t) {
    return *this;
}
Server& Server::set_write_timeout(time_t, time_t) {
    return *this;
}
Server& Server::set_idle_interval(time_t, time_t) {
    return *this;
}
Server& Server::set_payload_max_length(size_t) {
    return *this;
}
bool Server::process_request(Stream&, const std::string&, int, const std::string&, int, bool, bool&,
                             const std::function<void(Request&)>&) {
    return false;
}
bool Server::process_and_close_socket(socket_t) {
    return false;
}
std::unique_ptr<detail::MatcherBase> Server::make_matcher(const std::string&) {
    return nullptr;
}

} // namespace http
