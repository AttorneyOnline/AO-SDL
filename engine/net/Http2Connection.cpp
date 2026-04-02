#include "net/Http2Connection.h"

#include "utils/Log.h"

#include <nghttp2/nghttp2.h>

#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#endif

// ---------------------------------------------------------------------------
// Factory: connect with ALPN, return nullptr if h2 not negotiated
// ---------------------------------------------------------------------------

std::unique_ptr<Http2Connection> Http2Connection::connect(const std::string& host, uint16_t port,
                                                          int connection_timeout_ms) {

    auto socket = platform::tcp_connect(host, port, connection_timeout_ms);
    socket.ssl_connect(host, "h2,http/1.1");

    if (socket.negotiated_protocol() != "h2")
        return nullptr;

    return std::unique_ptr<Http2Connection>(new Http2Connection(std::move(socket), host));
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

Http2Connection::Http2Connection(platform::Socket socket, const std::string& host)
    : socket_(std::move(socket)), host_(host) {

    nghttp2_session_callbacks* callbacks = nullptr;
    nghttp2_session_callbacks_new(&callbacks);

    // All nghttp2 callbacks below access streams_ without explicitly acquiring
    // mutex_. This is safe because they are only invoked from
    // nghttp2_session_mem_recv2() inside pump_once(), which holds the lock.
    // Do NOT invoke nghttp2 session functions outside the mutex_ lock.

    nghttp2_session_callbacks_set_on_header_callback2(
        callbacks,
        [](nghttp2_session*, const nghttp2_frame* frame, nghttp2_rcbuf* name, nghttp2_rcbuf* value, uint8_t,
           void* user_data) -> int {
            auto* self = static_cast<Http2Connection*>(user_data);
            if (frame->hd.type != NGHTTP2_HEADERS || frame->headers.cat != NGHTTP2_HCAT_RESPONSE)
                return 0;
            auto nv = nghttp2_rcbuf_get_buf(name);
            auto vv = nghttp2_rcbuf_get_buf(value);
            if (nv.len == 7 && memcmp(nv.base, ":status", 7) == 0) {
                auto it = self->streams_.find(frame->hd.stream_id);
                if (it != self->streams_.end()) {
                    int status = 0;
                    std::from_chars(reinterpret_cast<const char*>(vv.base),
                                    reinterpret_cast<const char*>(vv.base) + vv.len, status);
                    it->second->response.status = status;
                }
            }
            return 0;
        });

    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(
        callbacks,
        [](nghttp2_session*, uint8_t, int32_t stream_id, const uint8_t* data, size_t len, void* user_data) -> int {
            auto* self = static_cast<Http2Connection*>(user_data);
            auto it = self->streams_.find(stream_id);
            if (it != self->streams_.end())
                it->second->response.body.append(reinterpret_cast<const char*>(data), len);
            return 0;
        });

    nghttp2_session_callbacks_set_on_stream_close_callback(
        callbacks, [](nghttp2_session*, int32_t stream_id, uint32_t error_code, void* user_data) -> int {
            auto* self = static_cast<Http2Connection*>(user_data);
            auto it = self->streams_.find(stream_id);
            if (it != self->streams_.end()) {
                if (error_code != 0)
                    it->second->response.error = "stream error " + std::to_string(error_code);
                if (it->second->callback)
                    it->second->callback(std::move(it->second->response));
                else
                    it->second->promise.set_value(std::move(it->second->response));
                self->streams_.erase(it);
            }
            return 0;
        });

    nghttp2_session_callbacks_set_on_frame_recv_callback(
        callbacks, [](nghttp2_session*, const nghttp2_frame* frame, void* user_data) -> int {
            if (frame->hd.type == NGHTTP2_GOAWAY) {
                auto* self = static_cast<Http2Connection*>(user_data);
                self->alive_ = false;
            }
            return 0;
        });

    nghttp2_session_client_new(&session_, callbacks, this);
    nghttp2_session_callbacks_del(callbacks);

    // Client SETTINGS: only set initial window size.
    // (MAX_CONCURRENT_STREAMS is a server-to-client setting, not useful here.)
    nghttp2_settings_entry settings[] = {
        {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, 1048576},
    };
    nghttp2_submit_settings(session_, NGHTTP2_FLAG_NONE, settings, 1);

    // The recv timeout controls I/O loop responsiveness. With Network.framework
    // on macOS this is a condition_variable wait; on other platforms it's SO_RCVTIMEO.
    socket_.set_recv_timeout(1);

    running_ = true;
    alive_ = true;
    io_thread_ = std::thread([this] { io_loop(); });
}

Http2Connection::~Http2Connection() {
    // shutdown() sets running_=false and calls socket_.shutdown(), which
    // breaks any blocking recv() in the I/O thread. The 1ms recv timeout
    // (set in constructor) ensures io_loop() exits promptly.
    // join() blocks until io_loop() returns, so socket_ and session_ are
    // safe to destroy afterwards — no thread is using them.
    shutdown();
    if (io_thread_.joinable())
        io_thread_.join();
    socket_.close();
    if (session_) {
        nghttp2_session_del(session_);
        session_ = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

static void build_get_headers(nghttp2_nv* hdrs, const std::string& path, const std::string& authority,
                              const std::string& priority_value) {
    hdrs[0] = {(uint8_t*)":method", (uint8_t*)"GET", 7, 3,
               NGHTTP2_NV_FLAG_NO_COPY_NAME | NGHTTP2_NV_FLAG_NO_COPY_VALUE};
    hdrs[1] = {(uint8_t*)":path", (uint8_t*)path.data(), 5, path.size(), NGHTTP2_NV_FLAG_NO_COPY_NAME};
    hdrs[2] = {(uint8_t*)":scheme", (uint8_t*)"https", 7, 5,
               NGHTTP2_NV_FLAG_NO_COPY_NAME | NGHTTP2_NV_FLAG_NO_COPY_VALUE};
    hdrs[3] = {(uint8_t*)":authority", (uint8_t*)authority.data(), 10, authority.size(), NGHTTP2_NV_FLAG_NO_COPY_NAME};
    hdrs[4] = {(uint8_t*)"priority", (uint8_t*)priority_value.data(), 8, priority_value.size(),
               NGHTTP2_NV_FLAG_NO_COPY_NAME};
}

std::future<Http2Connection::Response> Http2Connection::submit_get(const std::string& path, int urgency) {
    std::lock_guard lock(mutex_);

    if (!alive_) {
        std::promise<Response> p;
        p.set_value(Response{0, "", "connection closed"});
        return p.get_future();
    }

    std::string priority_value = "u=" + std::to_string(std::clamp(urgency, 0, 7));
    nghttp2_nv hdrs[5];
    build_get_headers(hdrs, path, host_, priority_value);

    auto sd = std::make_unique<StreamData>();
    auto future = sd->promise.get_future();

    int32_t id = nghttp2_submit_request2(session_, nullptr, hdrs, 5, nullptr, nullptr);
    if (id < 0) {
        sd->promise.set_value(Response{0, "", "submit failed: " + std::string(nghttp2_strerror(id))});
        return future;
    }

    streams_[id] = std::move(sd);
    return future;
}

void Http2Connection::submit_get_async(const std::string& path, int urgency, ResponseCallback on_complete) {
    std::lock_guard lock(mutex_);

    if (!alive_) {
        on_complete(Response{0, "", "connection closed"});
        return;
    }

    std::string priority_value = "u=" + std::to_string(std::clamp(urgency, 0, 7));
    nghttp2_nv hdrs[5];
    build_get_headers(hdrs, path, host_, priority_value);

    auto sd = std::make_unique<StreamData>();
    sd->callback = std::move(on_complete);

    int32_t id = nghttp2_submit_request2(session_, nullptr, hdrs, 5, nullptr, nullptr);
    if (id < 0) {
        sd->callback(Response{0, "", "submit failed: " + std::string(nghttp2_strerror(id))});
        return;
    }

    streams_[id] = std::move(sd);
}

bool Http2Connection::is_alive() const {
    return alive_;
}

void Http2Connection::shutdown() {
    alive_ = false;
    running_ = false;
    socket_.shutdown();
}

// ---------------------------------------------------------------------------
// I/O thread
// ---------------------------------------------------------------------------

void Http2Connection::io_loop() {
    send_pending();

    while (running_) {
        if (!pump_once())
            break;
    }

    alive_ = false;
    std::lock_guard lock(mutex_);
    for (auto& [id, sd] : streams_) {
        sd->response.error = "connection closed";
        if (sd->callback)
            sd->callback(std::move(sd->response));
        else
            sd->promise.set_value(std::move(sd->response));
    }
    streams_.clear();
}

bool Http2Connection::pump_once() {
    if (!running_)
        return false;

    send_pending();

    uint8_t buf[16384];
    ssize_t n = socket_.recv(buf, sizeof(buf));
    if (n > 0) {
        {
            std::lock_guard lock(mutex_);
            if (nghttp2_session_mem_recv2(session_, buf, static_cast<size_t>(n)) < 0) {
                alive_ = false;
                return false;
            }
        }
        send_pending();
    }
    else if (n == 0) {
        alive_ = false;
        return false;
    }

    if (!nghttp2_session_want_read(session_) && !nghttp2_session_want_write(session_)) {
        alive_ = false;
        return false;
    }

    return true;
}

void Http2Connection::send_pending() {
    std::lock_guard lock(mutex_);
    for (;;) {
        const uint8_t* data = nullptr;
        auto len = nghttp2_session_mem_send2(session_, &data);
        if (len <= 0) {
            if (len < 0)
                alive_ = false;
            break;
        }
        size_t total = 0;
        while (total < static_cast<size_t>(len)) {
            ssize_t sent = socket_.send(data + total, static_cast<size_t>(len) - total);
            if (sent <= 0) {
                alive_ = false;
                return;
            }
            total += static_cast<size_t>(sent);
        }
    }
}
