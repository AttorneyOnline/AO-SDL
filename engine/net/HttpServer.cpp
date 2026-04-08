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
#include "net/ProxyProtocol.h"
#include "net/SSEEvent.h"
#include "net/TrustedProxyList.h"

#include "event/EventChannel.h"
#include "event/EventManager.h"
#include "metrics/MetricsRegistry.h"
#include "platform/Poll.h"
#include "platform/Socket.h"
#include "utils/Log.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>

#ifndef _WIN32
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#endif

// -- SSE/HTTP metrics (file-scope — self-register at program startup) ---------

static auto& sse_events_ =
    metrics::MetricsRegistry::instance().counter("kagami_sse_events_total", "SSE events sent", {"event"});
static auto& sse_replays_ =
    metrics::MetricsRegistry::instance().counter("kagami_sse_replays_total", "SSE event replays on reconnect");
static auto& sse_dead_clients_ = metrics::MetricsRegistry::instance().counter(
    "kagami_sse_dead_clients_total", "SSE clients dropped due to send failure");
static auto& tcp_connections_ =
    metrics::MetricsRegistry::instance().counter("kagami_tcp_connections_total", "Total TCP connections accepted");
static auto& tcp_bytes_in_ = metrics::MetricsRegistry::instance().counter("kagami_tcp_bytes_received_total",
                                                                          "Total bytes read from TCP sockets");
static auto& tcp_bytes_out_ =
    metrics::MetricsRegistry::instance().counter("kagami_tcp_bytes_sent_total", "Total bytes written to TCP sockets");
static auto& tcp_retransmits_ = metrics::MetricsRegistry::instance().counter(
    "kagami_tcp_retransmits_total", "Total TCP retransmits (sampled on connection close)");

namespace http {

// ===========================================================================
// Internal types
// ===========================================================================

struct HttpConnection {
    uint64_t id;
    platform::Socket socket;
    std::string recv_buf;
    std::string remote_addr;            ///< Client IP address (may be overridden by proxy headers).
    bool proxy_protocol_parsed = false; ///< PROXY protocol header already processed on this connection.
    bool headers_complete = false;
    bool dispatched = false;   ///< Request has been dispatched to a worker thread.
    size_t header_end_pos = 0; ///< Byte offset of "\r\n\r\n" in recv_buf.
    size_t content_length = 0; ///< Expected body length from Content-Length header.
    bool keep_alive = false;
    // Async send state (io_uring completion mode)
    std::string pending_send;     ///< Response data kept alive during async send.
    bool send_keep_alive = false; ///< Keep-alive decision for pending send.
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
    bool request_keep_alive = false;
};

// ===========================================================================
// Server implementation
// ===========================================================================

struct Server::ServerState {
    platform::Poller poller;
    platform::Socket listener;
    int listener_fd = -1;
    int notifier_fd = -1;

    std::unordered_map<uint64_t, HttpConnection> connections;
    std::unordered_map<int, uint64_t> fd_to_conn; // fd → connection_id for O(1) lookup
    uint64_t next_conn_id = 1;

    // Private channels — no global EventManager contention
    EventChannel<WorkItem> work_channel;
    EventChannel<WorkResult> result_channel;

    // Worker pool
    std::vector<std::jthread> workers;
    std::mutex work_mutex;
    std::condition_variable_any work_cv;

    std::atomic<bool> running{false};

    // Connection count (atomic for cross-thread reads from metrics)
    std::atomic<int> open_connections{0};

    // Worker utilization metrics
    std::atomic<int> active_workers{0};
    size_t num_workers = 0;
    std::atomic<uint64_t> worker_idle_ns{0};
    std::atomic<uint64_t> worker_busy_ns{0};

    // Per-worker metrics (indexed by worker thread number)
    static constexpr size_t MAX_WORKERS = 64;
    enum WorkerSection { W_IDLE, W_DEQUEUE, W_HANDLER, W_RESULT, NUM_WORKER_SECTIONS };
    static constexpr const char* worker_section_names[] = {"idle", "dequeue", "handler", "result"};
    struct PerWorkerStats {
        std::atomic<uint64_t> idle_ns{0};
        std::atomic<uint64_t> busy_ns{0};
        std::atomic<uint64_t> section_ns[NUM_WORKER_SECTIONS] = {};
    };
    PerWorkerStats per_worker[MAX_WORKERS];

    // Poll thread utilization metrics
    std::atomic<uint64_t> poll_idle_ns{0};
    std::atomic<uint64_t> poll_busy_ns{0};
    std::atomic<uint64_t> poll_events_total{0};

    // Poll loop section profiling (cumulative nanoseconds per section)
    enum PollSection {
        ACCEPT,
        SSE_PUBLISH,
        RESPONSE_SEND,
        RECV,
        PARSE_DISPATCH,
        INLINE_HANDLER,
        SSE_KEEPALIVE,
        NUM_SECTIONS
    };
    static constexpr const char* poll_section_names[] = {"accept",         "sse_publish",    "response_send", "recv",
                                                         "parse_dispatch", "inline_handler", "sse_keepalive"};
    std::atomic<uint64_t> poll_section_ns[NUM_SECTIONS] = {};

    // Inline handlers — run on the poll thread, bypass the worker queue.
    // Use for endpoints that must stay responsive regardless of worker load
    // (e.g., /metrics, health checks).
    using HandlerEntry = std::pair<std::string, Server::Handler>;
    std::vector<HandlerEntry> inline_get_handlers;

    // Handler tables (dispatched to worker threads)
    std::vector<HandlerEntry> get_handlers;
    std::vector<HandlerEntry> post_handlers;
    std::vector<HandlerEntry> put_handlers;
    std::vector<HandlerEntry> patch_handlers;
    std::vector<HandlerEntry> delete_handlers;
    std::vector<HandlerEntry> options_handlers;

    Headers default_headers;

    // Reverse proxy support
    ReverseProxyConfig reverse_proxy_config;
    net::TrustedProxyList trusted_proxies;

    // SSE
    using SSEHandlerEntry = std::pair<std::string, Server::SSEHandler>;
    std::vector<SSEHandlerEntry> sse_handlers;

    struct SSEConnection {
        uint64_t id;
        platform::Socket socket;
        std::string area;
        std::string session_token; ///< For TTL refresh during keepalive
    };
    std::mutex sse_mutex; // Guards sse_by_fd — accessed from both poll thread and push_sse()
    std::unordered_map<int, SSEConnection> sse_by_fd; // fd → SSEConnection for O(1) lookup

    // Event ID sequencing for SSE reconnection support
    uint64_t next_event_id = 1; // guarded by sse_mutex

    // Ring buffer of recent SSE events for Last-Event-ID replay
    struct BufferedSSEEvent {
        uint64_t id;
        std::string event;
        std::string data;
        std::string area;
    };
    static constexpr size_t SSE_BUFFER_CAPACITY = 1024;
    std::deque<BufferedSSEEvent> sse_event_buffer; // guarded by sse_mutex

    // Keepalive timer
    std::chrono::steady_clock::time_point last_keepalive = std::chrono::steady_clock::now();

    // Session touch callback for keepalive TTL refresh
    Server::SSESessionTouchFunc sse_session_touch;
};

// ===========================================================================
// HTTP parsing helpers
// ===========================================================================

// -- Size limits (RFC 9112 §3, RFC 9112 §5) ----------------------------------

static constexpr size_t MAX_URI_LENGTH = 8192;
static constexpr size_t MAX_HEADER_SECTION_LENGTH = 65536;
static constexpr size_t MAX_BODY_LENGTH = 1048576; // 1 MiB

static bool find_header_end(const std::string& buf) {
    return buf.find("\r\n\r\n") != std::string::npos;
}

using ServerState = Server::ServerState;

/// Sample TCP retransmit count from the socket before closing it.
/// Returns 0 on unsupported platforms or if the call fails.
static uint32_t sample_tcp_retransmits([[maybe_unused]] int fd) {
#if defined(__linux__)
    struct tcp_info info{};
    socklen_t len = sizeof(info);
    if (getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, &len) == 0)
        return info.tcpi_total_retrans;
#elif defined(__APPLE__)
    struct tcp_connection_info info{};
    socklen_t len = sizeof(info);
    if (getsockopt(fd, IPPROTO_TCP, TCP_CONNECTION_INFO, &info, &len) == 0)
        return info.tcpi_txretransmitpackets;
#endif
    return 0;
}

static void remove_connection(ServerState& state, uint64_t conn_id) {
    auto it = state.connections.find(conn_id);
    if (it != state.connections.end()) {
        int fd = it->second.socket.fd();
        // Sample TCP retransmits before the socket is closed
        uint32_t retrans = sample_tcp_retransmits(fd);
        if (retrans > 0)
            tcp_retransmits_.get().inc(retrans);
        state.fd_to_conn.erase(fd);
        state.connections.erase(it);
        state.open_connections.fetch_sub(1, std::memory_order_relaxed);
    }
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

static std::string serialize_response(const Response& res, const Headers& default_headers, bool is_head = false,
                                      bool keep_alive = false) {
    std::string out;
    out.reserve(256 + (is_head ? 0 : res.body.size()));
    out += "HTTP/1.1 " + std::to_string(res.status) + " " + status_message(res.status) + "\r\n";
    out += keep_alive ? "Connection: keep-alive\r\n" : "Connection: close\r\n";

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

/// RAII guard that accumulates elapsed nanoseconds into an atomic counter.
struct SectionScope {
    std::atomic<uint64_t>& counter;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    ~SectionScope() {
        counter.fetch_add(
            static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count()),
            std::memory_order_relaxed);
    }
};

// ===========================================================================
// Worker thread loop
// ===========================================================================

static void worker_loop(Server::ServerState& state, std::stop_token st, size_t worker_id) {
    auto& pw = state.per_worker[worker_id < ServerState::MAX_WORKERS ? worker_id : 0];
    while (!st.stop_requested()) {
        // Wait for work (idle time)
        auto idle_start = std::chrono::steady_clock::now();
        {
            std::unique_lock lock(state.work_mutex);
            state.work_cv.wait(lock, st, [&] { return state.work_channel.has_events(); });
        }
        auto idle_end = std::chrono::steady_clock::now();
        auto idle_elapsed =
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(idle_end - idle_start).count());
        state.worker_idle_ns.fetch_add(idle_elapsed, std::memory_order_relaxed);
        pw.idle_ns.fetch_add(idle_elapsed, std::memory_order_relaxed);
        pw.section_ns[ServerState::W_IDLE].fetch_add(idle_elapsed, std::memory_order_relaxed);

        if (st.stop_requested())
            break;

        // Everything from here until the next idle wait is "busy" time
        auto busy_start = std::chrono::steady_clock::now();
        state.active_workers.fetch_add(1, std::memory_order_relaxed);

        // Dequeue
        std::optional<WorkItem> item;
        {
            SectionScope _s{pw.section_ns[ServerState::W_DEQUEUE]};
            item = state.work_channel.get_event();
        }
        if (!item) {
            state.active_workers.fetch_sub(1, std::memory_order_relaxed);
            continue;
        }

        // Handler execution
        Response res;
        res.status = 200;

        {
            SectionScope _s{pw.section_ns[ServerState::W_HANDLER]};
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
        }

        // Build and publish result
        {
            SectionScope _s{pw.section_ns[ServerState::W_RESULT]};
            WorkResult result;
            result.connection_id = item->connection_id;
            result.response = std::move(res);
            result.is_head = item->is_head;
            auto conn_hdr = item->request.get_header_value("Connection");
            result.request_keep_alive = detail::case_ignore::equal(conn_hdr, "keep-alive");
            state.result_channel.publish(std::move(result));
            state.poller.notify();
        }

        auto busy_elapsed = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - busy_start)
                .count());
        state.worker_busy_ns.fetch_add(busy_elapsed, std::memory_order_relaxed);
        pw.busy_ns.fetch_add(busy_elapsed, std::memory_order_relaxed);
        state.active_workers.fetch_sub(1, std::memory_order_relaxed);
    }
}

/// Send a raw frame string to an SSE socket. Returns false if the send fails.
/// Switches to blocking mode with a 5-second send timeout so that a stalled
/// client cannot block the poll loop indefinitely.
static bool send_sse_frame(platform::Socket& socket, const std::string& frame) {
    socket.set_non_blocking(false);
    socket.set_send_timeout(5000);
    size_t total = 0;
    bool ok = true;
    while (total < frame.size()) {
        ssize_t n = socket.send(frame.data() + total, frame.size() - total);
        if (n <= 0) {
            ok = false;
            break;
        }
        total += static_cast<size_t>(n);
    }
    socket.set_send_timeout(0);
    socket.set_non_blocking(true);
    return ok;
}

// -- Raw header field helpers (pre-parse, used during body accumulation) ------

/// Case-insensitive search for a header field in raw HTTP bytes.
/// \p name must be lowercase and include the trailing colon (e.g. "content-length:").
/// Returns the byte offset immediately after the colon, or std::string::npos.
static size_t find_raw_header_value(const std::string& raw, size_t header_end, std::string_view name) {
    for (size_t i = 0; i < header_end; ++i) {
        if (raw[i] != '\n' || i + 1 + name.size() > header_end)
            continue;
        size_t start = i + 1;
        bool match = true;
        for (size_t j = 0; j < name.size(); ++j) {
            if (std::tolower(static_cast<unsigned char>(raw[start + j])) != name[j]) {
                match = false;
                break;
            }
        }
        if (match)
            return start + name.size();
    }
    return std::string::npos;
}

/// Extract Content-Length from raw header bytes. Returns {true, value} if present
/// and valid, {false, 0} if absent. Callers should reject requests where
/// Content-Length is absent but a body is expected (411 Length Required).
static std::pair<bool, size_t> extract_content_length_raw(const std::string& raw, size_t header_end) {
    auto pos = find_raw_header_value(raw, header_end, "content-length:");
    if (pos == std::string::npos)
        return {false, 0};
    while (pos < header_end && (raw[pos] == ' ' || raw[pos] == '\t'))
        ++pos;
    size_t val = 0;
    while (pos < header_end && raw[pos] >= '0' && raw[pos] <= '9') {
        val = val * 10 + static_cast<size_t>(raw[pos] - '0');
        ++pos;
    }
    return {true, val};
}

/// Check if Transfer-Encoding header is present in raw header bytes.
static bool has_transfer_encoding_raw(const std::string& raw, size_t header_end) {
    return find_raw_header_value(raw, header_end, "transfer-encoding:") != std::string::npos;
}

// ===========================================================================
// Poll loop (runs on the thread that called listen_after_bind)
// ===========================================================================

static void poll_loop(Server* srv, Server::ServerState& state) {
    constexpr int MAX_EVENTS = 128;
    platform::Poller::Event events[MAX_EVENTS];

    while (state.running.load()) {
        auto poll_start = std::chrono::steady_clock::now();
        int n = state.poller.poll(events, MAX_EVENTS, 100);
        auto poll_end = std::chrono::steady_clock::now();
        // The time spent inside poll() is always idle — we were blocked in
        // the kernel waiting for events, not doing useful work.
        state.poll_idle_ns.fetch_add(
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(poll_end - poll_start).count()),
            std::memory_order_relaxed);
        if (n > 0)
            state.poll_events_total.fetch_add(static_cast<uint64_t>(n), std::memory_order_relaxed);

        for (int i = 0; i < n; ++i) {
            auto& ev = events[i];

            if (ev.fd == state.listener_fd) {
                SectionScope _s{state.poll_section_ns[ServerState::ACCEPT]};
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
                    conn.remote_addr = remote_addr;
                    conn.socket = std::move(client);

                    state.poller.add(cfd, platform::Poller::Readable);
                    state.fd_to_conn[cfd] = conn.id;
                    state.connections.emplace(conn.id, std::move(conn));
                    tcp_connections_.get().inc();
                    state.open_connections.fetch_add(1, std::memory_order_relaxed);
                }
            }
            else if (ev.fd == state.notifier_fd) {
                state.poller.drain_notifier();

                // Drain SSE events from global EventManager
                {
                    SectionScope _s{state.poll_section_ns[ServerState::SSE_PUBLISH]};
                    while (auto sse = EventManager::instance().get_channel<SSEEvent>().get_event()) {
                        srv->push_sse(sse->event, sse->data, sse->area);
                    }
                }

                // Process completed results
                {
                    SectionScope _s{state.poll_section_ns[ServerState::RESPONSE_SEND]};
                    while (auto result = state.result_channel.get_event()) {
                        auto conn_it = state.connections.find(result->connection_id);
                        if (conn_it == state.connections.end())
                            continue;
                        HttpConnection* conn = &conn_it->second;

                        // Determine keep-alive before serializing (so we can include
                        // the Connection header in the response).
                        auto resp_conn_hdr = result->response.get_header_value("Connection");
                        bool should_keep_alive =
                            result->request_keep_alive && !detail::case_ignore::equal(resp_conn_hdr, "close");

                        std::string response_data = serialize_response(result->response, state.default_headers,
                                                                       result->is_head, should_keep_alive);

                        // Move response into connection-owned storage so the
                        // buffer has a stable address for async send backends.
                        // submit_send() on io_uring stores the pointer and the
                        // kernel reads from it asynchronously — the data must
                        // outlive the SQE.
                        conn->pending_send = std::move(response_data);
                        conn->send_keep_alive = should_keep_alive;

                        size_t bytes_sent = 0;
                        bool sync = state.poller.submit_send(conn->socket.fd(), conn->pending_send.data(),
                                                             conn->pending_send.size(), &bytes_sent);

                        if (sync) {
                            // Readiness backend — send inline (blocking)
                            conn->socket.set_non_blocking(false);
                            conn->socket.set_send_timeout(5000);
                            size_t total = 0;
                            while (total < conn->pending_send.size()) {
                                ssize_t w = conn->socket.send(conn->pending_send.data() + total,
                                                              conn->pending_send.size() - total);
                                if (w <= 0)
                                    break;
                                total += static_cast<size_t>(w);
                            }
                            conn->socket.set_non_blocking(true);
                            tcp_bytes_out_.get().inc(total);

                            should_keep_alive = should_keep_alive && (total == conn->pending_send.size());
                            conn->pending_send.clear();
                            if (should_keep_alive) {
                                conn->headers_complete = false;
                                conn->dispatched = false;
                                conn->header_end_pos = 0;
                                conn->content_length = 0;
                                conn->recv_buf.clear();
                                conn->keep_alive = true;
                            }
                            else {
                                state.poller.remove(conn->socket.fd());
                                remove_connection(state, result->connection_id);
                            }
                        }
                        else {
                            // Completion backend — send is async.
                            // Data is already in conn->pending_send with a
                            // stable address. SendDone event handles cleanup.
                            conn->dispatched = true;
                        }
                    }
                } // end RESPONSE_SEND scope
            }
            else if (ev.flags & platform::Poller::SendDone) {
                // Async send completed (io_uring)
                SectionScope _s{state.poll_section_ns[ServerState::RESPONSE_SEND]};
                auto fd_it = state.fd_to_conn.find(ev.fd);
                if (fd_it == state.fd_to_conn.end())
                    continue;
                auto conn_it = state.connections.find(fd_it->second);
                if (conn_it == state.connections.end())
                    continue;
                HttpConnection* conn = &conn_it->second;

                tcp_bytes_out_.get().inc(ev.data_len);

                bool fully_sent = !(ev.flags & platform::Poller::Error) && ev.data_len == conn->pending_send.size();
                bool should_keep_alive = conn->send_keep_alive && fully_sent;
                conn->pending_send.clear();

                if (should_keep_alive) {
                    conn->headers_complete = false;
                    conn->dispatched = false;
                    conn->header_end_pos = 0;
                    conn->content_length = 0;
                    conn->recv_buf.clear();
                    conn->keep_alive = true;
                }
                else {
                    state.poller.remove(ev.fd);
                    remove_connection(state, conn->id);
                }
            }
            else {
                SectionScope _s{state.poll_section_ns[ServerState::RECV]};
                // O(1) SSE connection disconnect detection
                {
                    std::lock_guard sse_lock(state.sse_mutex);
                    auto sse_it = state.sse_by_fd.find(ev.fd);
                    if (sse_it != state.sse_by_fd.end()) {
                        if (ev.data_len > 0) {
                            // Completion mode: any data from SSE client is unexpected → recycle and check
                            state.poller.recycle_buffer(ev.buffer_id);
                        }
                        // In both modes, check for disconnect
                        bool disconnected = (ev.flags & platform::Poller::HangUp) ||
                                            (ev.data_len == 0 && !(ev.flags & platform::Poller::Readable));
                        if (!disconnected && ev.data_len == 0) {
                            // Readiness mode: probe with recv
                            char discard[64];
                            ssize_t n = sse_it->second.socket.recv(discard, sizeof(discard));
                            disconnected = (n <= 0);
                        }
                        if (disconnected) {
                            state.poller.remove(ev.fd);
                            state.sse_by_fd.erase(sse_it);
                        }
                        continue;
                    }
                }

                // O(1) fd → connection lookup
                auto fd_it = state.fd_to_conn.find(ev.fd);
                if (fd_it == state.fd_to_conn.end())
                    continue;
                auto conn_it = state.connections.find(fd_it->second);
                if (conn_it == state.connections.end())
                    continue;
                HttpConnection* conn = &conn_it->second;

                // Completion mode (io_uring): data already read by the kernel.
                // Readiness mode (epoll/kqueue): drain manually.
                bool closed = false;
                if (ev.data_len > 0) {
                    conn->recv_buf.append(static_cast<const char*>(ev.data), ev.data_len);
                    tcp_bytes_in_.get().inc(static_cast<uint64_t>(ev.data_len));
                    state.poller.recycle_buffer(ev.buffer_id);
                }
                else {
                    // Drain all available data (edge-triggered poller won't
                    // re-fire for data already in the kernel buffer).
                    char buf[8192];
                    while (true) {
                        ssize_t r = conn->socket.recv(buf, sizeof(buf));
                        if (r > 0) {
                            conn->recv_buf.append(buf, static_cast<size_t>(r));
                            tcp_bytes_in_.get().inc(static_cast<uint64_t>(r));
                        }
                        else {
                            if (r == 0)
                                closed = true;
                            break; // would-block or closed
                        }
                    }
                }
                if (closed && conn->recv_buf.empty()) {
                    state.poller.remove(ev.fd);
                    remove_connection(state, conn->id);
                    continue;
                }

                // PROXY protocol: parse before HTTP (once per connection)
                if (state.reverse_proxy_config.proxy_protocol && !conn->proxy_protocol_parsed &&
                    state.trusted_proxies.is_trusted(conn->remote_addr) && !conn->recv_buf.empty()) {
                    auto pp = net::parse_proxy_protocol(reinterpret_cast<const uint8_t*>(conn->recv_buf.data()),
                                                        conn->recv_buf.size());
                    if (pp.need_more_data)
                        continue; // Wait for more bytes
                    if (pp.valid && !pp.client_addr.empty())
                        conn->remote_addr = pp.client_addr;
                    if (pp.header_length > 0)
                        conn->recv_buf.erase(0, pp.header_length);
                    conn->proxy_protocol_parsed = true;
                }
                else if (!conn->proxy_protocol_parsed) {
                    conn->proxy_protocol_parsed = true;
                }

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
                                remove_connection(state, conn->id);
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
                        remove_connection(state, conn->id);
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
                    remove_connection(state, conn->id);
                    continue;
                }

                // -- Phase 1: Detect header completion and determine body length --
                // Runs once when the full header section (\r\n\r\n) first appears.
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
                        remove_connection(state, conn->id);
                        continue;
                    }

                    conn->header_end_pos = conn->recv_buf.find("\r\n\r\n");

                    // RFC 9112 §6.1: Transfer-Encoding takes precedence over
                    // Content-Length. Chunked framing is not implemented, so
                    // respond 411 (Length Required) to tell the client we need
                    // a Content-Length header instead.
                    if (has_transfer_encoding_raw(conn->recv_buf, conn->header_end_pos)) {
                        Response err_res;
                        err_res.status = 411;
                        err_res.set_content("Length Required", "text/plain");
                        std::string resp_data = serialize_response(err_res, state.default_headers);
                        conn->socket.set_non_blocking(false);
                        conn->socket.send(resp_data.data(), resp_data.size());
                        state.poller.remove(ev.fd);
                        remove_connection(state, conn->id);
                        continue;
                    }

                    // RFC 9112 §6.3: Determine body length from Content-Length.
                    // If neither Content-Length nor Transfer-Encoding is present,
                    // the body length is zero (RFC 9112 §6.3 step 7).
                    auto [has_cl, cl_value] = extract_content_length_raw(conn->recv_buf, conn->header_end_pos);
                    if (has_cl)
                        conn->content_length = cl_value;

                    // RFC 9110 §15.5.14: Reject oversized bodies early.
                    if (conn->content_length > MAX_BODY_LENGTH) {
                        Response err_res;
                        err_res.status = 413;
                        err_res.set_content("Content Too Large", "text/plain");
                        std::string resp_data = serialize_response(err_res, state.default_headers);
                        conn->socket.set_non_blocking(false);
                        conn->socket.send(resp_data.data(), resp_data.size());
                        state.poller.remove(ev.fd);
                        remove_connection(state, conn->id);
                        continue;
                    }
                }

                // -- Phase 2: Dispatch once the full body has been received -------
                // Re-checked on every poll event until enough bytes arrive.
                if (conn->headers_complete && !conn->dispatched) {
                    size_t body_start = conn->header_end_pos + 4;
                    size_t body_received = conn->recv_buf.size() > body_start ? conn->recv_buf.size() - body_start : 0;

                    if (body_received < conn->content_length) {
                        // Not enough body data yet.
                        if (closed) {
                            // Peer disconnected before sending the full body.
                            state.poller.remove(ev.fd);
                            remove_connection(state, conn->id);
                        }
                        continue;
                    }

                    conn->dispatched = true;

                    std::string headers_str = conn->recv_buf.substr(0, body_start);
                    std::string body = conn->recv_buf.substr(body_start, conn->content_length);

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
                        remove_connection(state, conn->id);
                        continue;
                    }

                    // Check SSE handlers first (GET only, before normal dispatch)
                    if (req.method == "GET") {
                        bool handled_as_sse = false;
                        for (auto& [pattern, sse_handler] : state.sse_handlers) {
                            detail::PathParamsMatcher matcher(pattern);
                            if (pattern == req.path || matcher.match(req)) {
                                Response res;
                                res.status = 200;
                                auto result = sse_handler(req, res);
                                if (result.accepted) {
                                    Log::log_print(VERBOSE, "SSE: client connected on %s", req.path.c_str());
                                    // Accepted — send SSE headers and move to SSE connections
                                    std::string headers = "HTTP/1.1 200 OK\r\n"
                                                          "Content-Type: text/event-stream\r\n"
                                                          "Cache-Control: no-cache\r\n"
                                                          "Connection: keep-alive\r\n";
                                    headers += "Date: " + generate_date_header() + "\r\n";
                                    for (auto& [k, v] : state.default_headers)
                                        headers += k + ": " + v + "\r\n";
                                    headers += "\r\n";
                                    conn->socket.set_non_blocking(false);
                                    conn->socket.send(headers.data(), headers.size());
                                    conn->socket.set_non_blocking(true);

                                    // Extract area from query params
                                    std::string area;
                                    auto area_it = req.params.find("area");
                                    if (area_it != req.params.end())
                                        area = area_it->second;

                                    ServerState::SSEConnection sse_conn;
                                    sse_conn.id = conn->id;
                                    sse_conn.socket = std::move(conn->socket);
                                    sse_conn.area = area;
                                    sse_conn.session_token = std::move(result.session_token);

                                    // Replay buffered events if client sent Last-Event-ID
                                    uint64_t last_id = 0;
                                    auto leid = req.get_header_value("Last-Event-ID");
                                    if (!leid.empty()) {
                                        try {
                                            last_id = std::stoull(leid);
                                        }
                                        catch (...) {
                                            Log::log_print(DEBUG, "SSE: ignoring malformed Last-Event-ID: %s",
                                                           leid.c_str());
                                        }
                                    }

                                    // Copy replay frames under lock, then send outside it.
                                    // The socket isn't in sse_by_fd yet, so no concurrent access.
                                    std::vector<std::string> replay_frames;
                                    int sse_fd = sse_conn.socket.fd();
                                    {
                                        std::lock_guard sse_lock(state.sse_mutex);
                                        if (last_id > 0) {
                                            for (auto& buf : state.sse_event_buffer) {
                                                if (buf.id <= last_id)
                                                    continue;
                                                if (!buf.area.empty() && !area.empty() && buf.area != area)
                                                    continue;
                                                replay_frames.push_back("id: " + std::to_string(buf.id) + "\nevent: " +
                                                                        buf.event + "\ndata: " + buf.data + "\n\n");
                                            }
                                        }
                                    }

                                    // Send replay frames without holding the lock
                                    if (!replay_frames.empty()) {
                                        sse_replays_.get().inc(replay_frames.size());
                                        Log::log_print(VERBOSE, "SSE: replaying %zu events from id %lu",
                                                       replay_frames.size(), (unsigned long)last_id);
                                    }
                                    for (auto& frame : replay_frames)
                                        send_sse_frame(sse_conn.socket, frame);

                                    // Now add to live set
                                    {
                                        std::lock_guard sse_lock(state.sse_mutex);
                                        state.sse_by_fd.emplace(sse_fd, std::move(sse_conn));
                                    }
                                    // Remove from normal connections (fd_to_conn already points here)
                                    state.fd_to_conn.erase(ev.fd);
                                    state.connections.erase(conn->id);
                                }
                                else {
                                    // Rejected by handler
                                    std::string resp_data = serialize_response(res, state.default_headers);
                                    conn->socket.set_non_blocking(false);
                                    conn->socket.send(resp_data.data(), resp_data.size());
                                    state.poller.remove(ev.fd);
                                    remove_connection(state, conn->id);
                                }
                                handled_as_sse = true;
                                break;
                            }
                        }
                        if (handled_as_sse)
                            continue;
                    }

                    // Track if this is a HEAD request
                    bool is_head = (req.method == "HEAD");

                    // Check inline handlers first — these run on the poll
                    // thread and bypass the worker queue entirely. Used for
                    // endpoints that must stay responsive under worker saturation.
                    if (req.method == "GET" || req.method == "HEAD") {
                        auto inline_handler = find_handler(state.inline_get_handlers, req.path, req);
                        if (inline_handler) {
                            Response res;
                            res.status = 200;
                            try {
                                inline_handler(req, res);
                            }
                            catch (const std::exception& e) {
                                res.status = 500;
                                res.set_content("Internal Server Error", "text/plain");
                                Log::log_print(ERR, "Inline handler exception: %s", e.what());
                            }
                            // Send response directly from the poll thread
                            std::string resp_data = serialize_response(res, state.default_headers, is_head, false);
                            conn->socket.set_non_blocking(false);
                            conn->socket.send(resp_data.data(), resp_data.size());
                            conn->socket.set_non_blocking(true);
                            tcp_bytes_out_.get().inc(resp_data.size());
                            // Close connection (inline handlers don't support keep-alive)
                            state.poller.remove(ev.fd);
                            remove_connection(state, conn->id);
                            continue;
                        }
                    }

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

                    // Populate request remote_addr from connection
                    req.remote_addr = conn->remote_addr;

                    // Extract real client IP from proxy headers if configured
                    if (state.reverse_proxy_config.enabled && state.trusted_proxies.is_trusted(conn->remote_addr)) {
                        for (auto& hdr : state.reverse_proxy_config.header_priority) {
                            auto val = req.get_header_value(hdr);
                            if (!val.empty()) {
                                auto real_ip = net::extract_forwarded_ip(val, &state.trusted_proxies);
                                if (!real_ip.empty()) {
                                    req.remote_addr = real_ip;
                                    break;
                                }
                            }
                        }
                    }

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

        // --- SSE keepalive (every ~30s) ---
        auto now = std::chrono::steady_clock::now();
        if (now - state.last_keepalive > std::chrono::seconds(30)) {
            SectionScope _s{state.poll_section_ns[ServerState::SSE_KEEPALIVE]};
            state.last_keepalive = now;
            std::lock_guard sse_lock(state.sse_mutex);
            std::vector<int> dead_fds;
            for (auto& [fd, sse] : state.sse_by_fd) {
                if (!send_sse_frame(sse.socket, ":keepalive\n\n"))
                    dead_fds.push_back(fd);
                else if (!sse.session_token.empty() && state.sse_session_touch)
                    state.sse_session_touch(sse.session_token);
            }
            for (int fd : dead_fds) {
                state.poller.remove(fd);
                state.sse_by_fd.erase(fd);
            }
        }

        // Attribute all post-poll work (event handling + SSE keepalive) as busy
        state.poll_busy_ns.fetch_add(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                               std::chrono::steady_clock::now() - poll_end)
                                                               .count()),
                                     std::memory_order_relaxed);
    }
}

// ===========================================================================
// Server public methods
// ===========================================================================

Server::Server() : state_(std::make_unique<ServerState>()) {
}

Server::~Server() {
    stop();
}

bool Server::is_valid() const {
    return true;
}

// Route registration — store handler with pattern string
Server& Server::Get(const std::string& pattern, Handler handler) {
    state_->get_handlers.emplace_back(pattern, std::move(handler));
    return *this;
}

Server& Server::GetInline(const std::string& pattern, Handler handler) {
    state_->inline_get_handlers.emplace_back(pattern, std::move(handler));
    return *this;
}

Server& Server::Post(const std::string& pattern, Handler handler) {
    state_->post_handlers.emplace_back(pattern, std::move(handler));
    return *this;
}

Server& Server::Post(const std::string&, HandlerWithContentReader) {
    return *this;
}

Server& Server::Put(const std::string& pattern, Handler handler) {
    state_->put_handlers.emplace_back(pattern, std::move(handler));
    return *this;
}

Server& Server::Put(const std::string&, HandlerWithContentReader) {
    return *this;
}

Server& Server::Patch(const std::string& pattern, Handler handler) {
    state_->patch_handlers.emplace_back(pattern, std::move(handler));
    return *this;
}

Server& Server::Patch(const std::string&, HandlerWithContentReader) {
    return *this;
}

Server& Server::Delete(const std::string& pattern, Handler handler) {
    state_->delete_handlers.emplace_back(pattern, std::move(handler));
    return *this;
}

Server& Server::Delete(const std::string&, HandlerWithContentReader) {
    return *this;
}

Server& Server::Options(const std::string& pattern, Handler handler) {
    state_->options_handlers.emplace_back(pattern, std::move(handler));
    return *this;
}

Server& Server::SSE(const std::string& pattern, SSEHandler handler) {
    state_->sse_handlers.emplace_back(pattern, std::move(handler));
    return *this;
}

void Server::set_sse_session_touch(SSESessionTouchFunc func) {
    if (state_)
        state_->sse_session_touch = std::move(func);
}

void Server::push_sse(const std::string& event, const std::string& data, const std::string& area) {
    if (!state_)
        return;

    std::lock_guard lock(state_->sse_mutex);

    sse_events_.labels({event}).inc();

    // Assign monotonic event ID
    uint64_t eid = state_->next_event_id++;
    std::string frame = "id: " + std::to_string(eid) + "\nevent: " + event + "\ndata: " + data + "\n\n";

    // Buffer for reconnection replay
    state_->sse_event_buffer.push_back({eid, event, data, area});
    if (state_->sse_event_buffer.size() > ServerState::SSE_BUFFER_CAPACITY)
        state_->sse_event_buffer.pop_front();

    // Send to all matching connections
    std::vector<int> dead_fds;
    int sent_count = 0;
    for (auto& [fd, sse] : state_->sse_by_fd) {
        if (!area.empty() && !sse.area.empty() && sse.area != area)
            continue;
        if (!send_sse_frame(sse.socket, frame))
            dead_fds.push_back(fd);
        else
            ++sent_count;
    }
    Log::log_print(VERBOSE, "SSE: push id=%lu %s -> %d client(s)", (unsigned long)eid, event.c_str(), sent_count);
    if (!dead_fds.empty())
        sse_dead_clients_.get().inc(dead_fds.size());
    for (int fd : dead_fds) {
        Log::log_print(VERBOSE, "SSE: dropped dead fd=%d", fd);
        state_->poller.remove(fd);
        state_->sse_by_fd.erase(fd);
    }
}

// Configuration
Server& Server::set_default_headers(Headers headers) {
    state_->default_headers = std::move(headers);
    default_headers_ = state_->default_headers;
    return *this;
}

bool Server::bind_to_port(const std::string& host, int port, int) {
    auto& state = *state_;
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

    auto& state = *state_;
    uint16_t port = state.listener.local_port();
    return (port > 0) ? static_cast<int>(port) : -1;
}

bool Server::listen_after_bind() {
    auto& state = *state_;
    state.running.store(true);
    is_running_.store(true);

    // Wire SSE event channel to poller notification
    EventManager::instance().get_channel<SSEEvent>().set_on_publish([&state] { state.poller.notify(); });

    // Start worker threads
    size_t num_workers = CPPHTTPLIB_THREAD_POOL_COUNT;
    state.num_workers = num_workers;
    for (size_t i = 0; i < num_workers; ++i) {
        state.workers.emplace_back([&state, i](std::stop_token st) { worker_loop(state, st, i); });
    }

    // Run poll loop on this thread (blocks)
    poll_loop(this, state);

    // Cleanup
    EventManager::instance().get_channel<SSEEvent>().set_on_publish(nullptr);
    for (auto& w : state.workers)
        w.request_stop();
    state.work_cv.notify_all();
    state.workers.clear();
    {
        std::lock_guard sse_lock(state.sse_mutex);
        state.sse_by_fd.clear();
    }

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
    auto& state = *state_;
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
Server& Server::set_reverse_proxy_config(const ReverseProxyConfig& config) {
    if (state_) {
        state_->reverse_proxy_config = config;
        state_->trusted_proxies.configure(config.trusted_proxies);
    }
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

// -- Observable internals (for metrics) --------------------------------------

size_t Server::open_connections() const {
    return state_ ? static_cast<size_t>(std::max(0, state_->open_connections.load(std::memory_order_relaxed))) : 0;
}

size_t Server::work_queue_depth() const {
    return state_ ? state_->work_channel.size() : 0;
}

size_t Server::result_queue_depth() const {
    return state_ ? state_->result_channel.size() : 0;
}

int Server::active_workers() const {
    return state_ ? state_->active_workers.load(std::memory_order_relaxed) : 0;
}

size_t Server::worker_count() const {
    return state_ ? state_->num_workers : 0;
}

uint64_t Server::worker_idle_ns() const {
    return state_ ? state_->worker_idle_ns.load(std::memory_order_relaxed) : 0;
}

uint64_t Server::worker_busy_ns() const {
    return state_ ? state_->worker_busy_ns.load(std::memory_order_relaxed) : 0;
}

uint64_t Server::poll_idle_ns() const {
    return state_ ? state_->poll_idle_ns.load(std::memory_order_relaxed) : 0;
}

uint64_t Server::poll_busy_ns() const {
    return state_ ? state_->poll_busy_ns.load(std::memory_order_relaxed) : 0;
}

uint64_t Server::poll_events_total() const {
    return state_ ? state_->poll_events_total.load(std::memory_order_relaxed) : 0;
}

size_t Server::poll_section_count() const {
    return ServerState::NUM_SECTIONS;
}

const char* Server::poll_section_name(size_t i) const {
    return i < ServerState::NUM_SECTIONS ? ServerState::poll_section_names[i] : "unknown";
}

uint64_t Server::poll_section_ns(size_t i) const {
    if (!state_ || i >= ServerState::NUM_SECTIONS)
        return 0;
    return state_->poll_section_ns[i].load(std::memory_order_relaxed);
}

size_t Server::worker_section_count() const {
    return ServerState::NUM_WORKER_SECTIONS;
}

const char* Server::worker_section_name(size_t i) const {
    return i < ServerState::NUM_WORKER_SECTIONS ? ServerState::worker_section_names[i] : "unknown";
}

uint64_t Server::worker_section_ns(size_t w, size_t s) const {
    if (!state_ || w >= state_->num_workers || s >= ServerState::NUM_WORKER_SECTIONS)
        return 0;
    return state_->per_worker[w].section_ns[s].load(std::memory_order_relaxed);
}

platform::Poller::IoStats Server::io_stats() const {
    return state_ ? state_->poller.io_stats() : platform::Poller::IoStats{};
}

std::unique_ptr<detail::MatcherBase> Server::make_matcher(const std::string&) {
    return nullptr;
}

} // namespace http
