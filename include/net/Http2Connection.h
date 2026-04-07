#pragma once

#include "platform/Socket.h"

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

struct nghttp2_session;

/// A single HTTP/2 multiplexed connection over a TLS socket.
///
/// Multiple threads can call submit() concurrently — each request becomes
/// a stream on the shared connection. A background I/O thread pumps the
/// nghttp2 session, reading/writing the socket and fulfilling promises
/// when streams complete.
///
/// If the connection drops, submit() returns an error. The caller (HttpPool)
/// is responsible for creating a new Http2Connection or falling back to h1.
class Http2Connection {
  public:
    struct Response {
        int status = 0;
        std::string body;
        std::string error;
    };

    /// Connect and perform TLS+ALPN handshake. Returns nullptr if h2 not negotiated.
    /// Throws on connection failure.
    static std::unique_ptr<Http2Connection> connect(const std::string& host, uint16_t port,
                                                    int connection_timeout_ms = -1);

    ~Http2Connection();

    Http2Connection(const Http2Connection&) = delete;
    Http2Connection& operator=(const Http2Connection&) = delete;

    /// Submit a GET request. Returns a future that resolves when the response is complete.
    /// Thread-safe — multiple threads can submit concurrently.
    /// @param urgency  RFC 9218 urgency: 0 = highest, 7 = lowest. Default 3.
    std::future<Response> submit_get(const std::string& path, int urgency = 3);

    /// Submit a GET request with a completion callback (called from I/O thread).
    /// This avoids blocking a worker thread and enables full multiplexing.
    using ResponseCallback = std::function<void(Response)>;
    void submit_get_async(const std::string& path, int urgency, ResponseCallback on_complete);

    /// Submit a POST request with a body. Returns a future.
    std::future<Response> submit_post(const std::string& path, const std::string& body, const std::string& content_type,
                                      const std::vector<std::pair<std::string, std::string>>& extra_headers = {},
                                      int urgency = 3);

    /// Submit a POST request with a completion callback.
    void submit_post_async(const std::string& path, const std::string& body, const std::string& content_type,
                           const std::vector<std::pair<std::string, std::string>>& extra_headers, int urgency,
                           ResponseCallback on_complete);

    /// True if the connection is still alive and accepting new streams.
    bool is_alive() const;

    /// Shut down the connection gracefully.
    void shutdown();

    // StreamData is public so the nghttp2 data provider callback can access it.
    struct StreamData {
        std::promise<Response> promise;
        ResponseCallback callback;
        Response response;
        std::string request_body; // kept alive for POST body data provider
        size_t body_offset = 0;
    };

  private:
    Http2Connection(platform::Socket socket, const std::string& host);

    void io_loop();
    bool pump_once();
    void send_pending();

    platform::Socket socket_;
    std::string host_;
    nghttp2_session* session_ = nullptr;

    std::mutex mutex_; // protects streams_ and session_ submit/recv calls
    std::unordered_map<int32_t, std::unique_ptr<StreamData>> streams_;

    std::thread io_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> alive_{false};
};
