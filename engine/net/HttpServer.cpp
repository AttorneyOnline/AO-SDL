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
    bool is_head = false;
};

struct WorkResult : public Event {
    uint64_t connection_id;
    Response response;
    bool is_head = false;
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

// -- Size limits (RFC 9112 §3, RFC 9112 §5) ----------------------------------

static constexpr size_t MAX_URI_LENGTH = 8192;
static constexpr size_t MAX_HEADER_SECTION_LENGTH = 65536;

static bool find_header_end(const std::string& buf) {
    return buf.find("\r\n\r\n") != std::string::npos;
}

static Server::Handler find_handler(const std::vector<ServerState::HandlerEntry>& handlers, const std::string& path,
                                    Request& req) {
    for (auto& [pattern, handler] : handlers) {
        if (pattern == path)
            return handler;
        detail::PathParamsMatcher matcher(pattern);
        if (matcher.match(req))
            return handler;
    }
    return nullptr;
}

// -- RFC-compliant request validation -----------------------------------------

/// Return non-zero HTTP error status if the raw request is malformed, 0 if ok.
static int validate_raw_request(const std::string& raw) {
    auto header_end = raw.find("\r\n\r\n");
    if (header_end == std::string::npos)
        return 0; // incomplete, not an error yet

    // RFC 9112 §3: URI Too Long
    auto first_line_end = raw.find("\r\n");
    if (first_line_end != std::string::npos) {
        std::string req_line = raw.substr(0, first_line_end);
        auto sp1 = req_line.find(' ');
        auto sp2 = (sp1 != std::string::npos) ? req_line.find(' ', sp1 + 1) : std::string::npos;
        if (sp1 != std::string::npos && sp2 != std::string::npos) {
            size_t uri_len = sp2 - sp1 - 1;
            if (uri_len > MAX_URI_LENGTH)
                return 414;
        }
    }

    // RFC 9112 §5: Header section too large
    if (header_end > MAX_HEADER_SECTION_LENGTH)
        return 431;

    // RFC 9112 §5.2: obs-fold detection — scan for CRLF followed by SP/HTAB
    // in the header section (after request-line, before empty line)
    std::string header_section = raw.substr(0, header_end);
    for (size_t i = 0; i + 2 < header_section.size(); ++i) {
        if (header_section[i] == '\r' && header_section[i + 1] == '\n') {
            if (i + 2 < header_section.size()) {
                char next = header_section[i + 2];
                // Skip the request-line terminator — obs-fold only applies to headers
                if (i < first_line_end)
                    continue;
                if (next == ' ' || next == '\t')
                    return 400;
            }
        }
    }

    // RFC 9112 §5.1: whitespace between field name and colon
    size_t pos = (first_line_end != std::string::npos) ? first_line_end + 2 : 0;
    while (pos < header_section.size()) {
        auto line_end = header_section.find("\r\n", pos);
        if (line_end == std::string::npos || line_end == pos)
            break;
        std::string line = header_section.substr(pos, line_end - pos);

        auto colon = line.find(':');
        if (colon != std::string::npos && colon > 0) {
            if (line[colon - 1] == ' ' || line[colon - 1] == '\t') {
                return 400;
            }
        }
        pos = line_end + 2;
    }

    return 0;
}

static int validate_parsed_request(const Request& req) {
    // RFC 9112 §3.2: Missing Host header in HTTP/1.1
    if (req.version == "HTTP/1.1") {
        size_t host_count = req.get_header_value_count("Host");
        if (host_count == 0)
            return 400;
        if (host_count > 1)
            return 400; // duplicate Host
    }

    // RFC 9112 §6.2: Invalid Content-Length
    auto cl_val = req.get_header_value("Content-Length", "");
    if (!cl_val.empty()) {
        for (char c : cl_val) {
            if (c < '0' || c > '9')
                return 400;
        }
    }

    // RFC 9112 §6.1: Transfer-Encoding present but chunked is not the final encoding
    auto te_val = req.get_header_value("Transfer-Encoding", "");
    if (!te_val.empty()) {
        // The final encoding must be "chunked"
        std::string te = te_val;
        // Trim trailing whitespace
        while (!te.empty() && (te.back() == ' ' || te.back() == '\t'))
            te.pop_back();
        // Get the last comma-separated value
        auto last_comma = te.rfind(',');
        std::string final_te = (last_comma != std::string::npos) ? te.substr(last_comma + 1) : te;
        while (!final_te.empty() && final_te[0] == ' ')
            final_te.erase(final_te.begin());
        if (final_te != "chunked")
            return 400;
    }

    return 0;
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

    // Handle absolute-form URIs: "http://host/path" → extract "/path"
    if (req.path.find("://") != std::string::npos) {
        auto scheme_end = req.path.find("://");
        auto path_start = req.path.find('/', scheme_end + 3);
        if (path_start != std::string::npos) {
            req.path = req.path.substr(path_start);
        }
        else {
            req.path = "/";
        }
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
            // RFC 9110 §5.5: Trim leading and trailing OWS from field values
            while (!val.empty() && (val.front() == ' ' || val.front() == '\t'))
                val.erase(val.begin());
            while (!val.empty() && (val.back() == ' ' || val.back() == '\t'))
                val.pop_back();
            req.headers.emplace(std::move(key), std::move(val));
        }
        pos = line_end + 2;
    }

    req.body = body;
    req.target = req.path;

    return req;
}

// -- RFC 9110 §6.6.1: Date header (IMF-fixdate) ------------------------------

static std::string generate_date_header() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    struct tm gmt{};
#ifdef _WIN32
    gmtime_s(&gmt, &t);
#else
    gmtime_r(&t, &gmt);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &gmt);
    return buf;
}

static std::string serialize_response(const Response& res, const Headers& default_headers, bool is_head = false) {
    std::string out;
    out.reserve(256 + (is_head ? 0 : res.body.size()));
    out += "HTTP/1.1 " + std::to_string(res.status) + " " + status_message(res.status) + "\r\n";

    // RFC 9110 §6.6.1: Date header on 2xx, 3xx, 4xx responses
    if (res.status >= 200 && res.status < 500) {
        out += "Date: " + generate_date_header() + "\r\n";
    }

    // Default headers
    for (auto& [k, v] : default_headers) {
        out += k + ": " + v + "\r\n";
    }

    // Response headers
    for (auto& [k, v] : res.headers) {
        out += k + ": " + v + "\r\n";
    }

    // RFC 9110 §8.6: No Content-Length in 204
    if (res.status != 204 && !res.body.empty()) {
        out += "Content-Length: " + std::to_string(res.body.size()) + "\r\n";
    }

    out += "\r\n";

    // RFC 9110 §9.3.2: HEAD response MUST NOT contain a body
    if (!is_head) {
        out += res.body;
    }
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
        result.is_head = item->is_head;
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

                    std::string response_data =
                        serialize_response(result->response, state.default_headers, result->is_head);

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

                // RFC 9112 §3: URI length check — early reject before headers complete
                if (!conn->headers_complete) {
                    auto first_line_end = conn->recv_buf.find("\r\n");
                    if (first_line_end != std::string::npos) {
                        std::string req_line = conn->recv_buf.substr(0, first_line_end);
                        auto sp1 = req_line.find(' ');
                        auto sp2 = (sp1 != std::string::npos) ? req_line.find(' ', sp1 + 1) : std::string::npos;
                        if (sp1 != std::string::npos && sp2 != std::string::npos) {
                            size_t uri_len = sp2 - sp1 - 1;
                            if (uri_len > MAX_URI_LENGTH) {
                                Response err_res;
                                err_res.status = 414;
                                err_res.set_content("URI Too Long", "text/plain");
                                std::string resp_data = serialize_response(err_res, state.default_headers);
                                conn->socket.set_non_blocking(false);
                                conn->socket.send(resp_data.data(), resp_data.size());
                                state.poller.remove(ev.fd);
                                state.connections.erase(conn->id);
                                continue;
                            }
                        }
                    }
                    else if (conn->recv_buf.size() > MAX_URI_LENGTH + 100) {
                        // Haven't even seen the first \r\n yet and buffer is huge
                        Response err_res;
                        err_res.status = 414;
                        err_res.set_content("URI Too Long", "text/plain");
                        std::string resp_data = serialize_response(err_res, state.default_headers);
                        conn->socket.set_non_blocking(false);
                        conn->socket.send(resp_data.data(), resp_data.size());
                        state.poller.remove(ev.fd);
                        state.connections.erase(conn->id);
                        continue;
                    }
                }

                // RFC 9112 §5: Header section size limit
                if (conn->recv_buf.size() > MAX_HEADER_SECTION_LENGTH && !find_header_end(conn->recv_buf)) {
                    Response err_res;
                    err_res.status = 431;
                    err_res.set_content("Request Header Fields Too Large", "text/plain");
                    std::string resp_data = serialize_response(err_res, state.default_headers);
                    conn->socket.set_non_blocking(false);
                    conn->socket.send(resp_data.data(), resp_data.size());
                    conn->socket.shutdown();
                    state.poller.remove(ev.fd);
                    state.connections.erase(conn->id);
                    continue;
                }

                // Check if headers are complete
                if (!conn->headers_complete && find_header_end(conn->recv_buf)) {
                    conn->headers_complete = true;

                    // RFC validation on raw bytes (before parsing)
                    int raw_err = validate_raw_request(conn->recv_buf);
                    if (raw_err != 0) {
                        Response err_res;
                        err_res.status = raw_err;
                        err_res.set_content(status_message(raw_err), "text/plain");
                        std::string resp_data = serialize_response(err_res, state.default_headers);
                        conn->socket.set_non_blocking(false);
                        conn->socket.send(resp_data.data(), resp_data.size());
                        state.poller.remove(ev.fd);
                        state.connections.erase(conn->id);
                        continue;
                    }

                    auto header_end_pos = conn->recv_buf.find("\r\n\r\n");
                    std::string headers_str = conn->recv_buf.substr(0, header_end_pos + 4);
                    std::string body = conn->recv_buf.substr(header_end_pos + 4);

                    Request req = parse_http_request(headers_str, body);

                    // RFC validation on parsed request
                    int parsed_err = validate_parsed_request(req);
                    if (parsed_err != 0) {
                        Response err_res;
                        err_res.status = parsed_err;
                        err_res.set_content(status_message(parsed_err), "text/plain");
                        std::string resp_data = serialize_response(err_res, state.default_headers);
                        conn->socket.set_non_blocking(false);
                        conn->socket.send(resp_data.data(), resp_data.size());
                        state.poller.remove(ev.fd);
                        state.connections.erase(conn->id);
                        continue;
                    }

                    // Track if this is a HEAD request
                    bool is_head = (req.method == "HEAD");

                    // Find handler — HEAD dispatches to GET handlers (RFC 9110 §9.3.2)
                    Server::Handler handler;
                    if (req.method == "GET" || req.method == "HEAD")
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
                    item.is_head = is_head;
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
