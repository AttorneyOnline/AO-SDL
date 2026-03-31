/**
 * @file apple_httplib.h
 * @brief Drop-in httplib::Client replacement using NSURLSession for iOS/macOS.
 *
 * Provides the exact API surface used by HttpPool.cpp:
 *   - Client(host), set_connection_timeout, set_read_timeout, set_keep_alive
 *   - Get(path) → Result with .status, .body
 *   - Get(path, chunk_callback) → streaming variant
 *   - Error enum, to_string(Error)
 *
 * This replaces cpp-httplib on Apple platforms where OpenSSL is unavailable.
 * NSURLSession handles TLS via Apple's native SecureTransport/Network.framework.
 */
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>

namespace httplib {

enum class Error {
    Success = 0,
    Connection,
    Read,
    Write,
    Canceled,
    SSLConnection,
    SSLLoadingCerts,
    Unknown,
};

inline const char* to_string(Error err) {
    switch (err) {
    case Error::Success:
        return "Success";
    case Error::Connection:
        return "Connection error";
    case Error::Read:
        return "Read error";
    case Error::Write:
        return "Write error";
    case Error::Canceled:
        return "Canceled";
    case Error::SSLConnection:
        return "SSL connection error";
    case Error::SSLLoadingCerts:
        return "SSL certificate error";
    default:
        return "Unknown error";
    }
}

struct Response {
    int status = 0;
    std::string body;
    // Server-side stub (unused on iOS, needed for RestRouter compilation)
    void set_content(const std::string& b, const std::string& /*content_type*/) {
        body = b;
    }
    void set_header(const std::string& /*key*/, const std::string& /*value*/) {
    }
    std::string get_header_value(const std::string& /*key*/) const {
        return {};
    }
};

/// Result wraps a Response or an Error, matching httplib::Result's interface.
class Result {
  public:
    Result() : error_(Error::Unknown) {
    }
    Result(Response resp) : response_(std::make_unique<Response>(std::move(resp))), error_(Error::Success) {
    }
    Result(Error err) : error_(err) {
    }

    explicit operator bool() const {
        return error_ == Error::Success && response_;
    }
    Response* operator->() {
        return response_.get();
    }
    const Response* operator->() const {
        return response_.get();
    }
    Error error() const {
        return error_;
    }

  private:
    std::unique_ptr<Response> response_;
    Error error_;
};

/// Minimal httplib::Client replacement using NSURLSession.
class Client {
  public:
    explicit Client(const std::string& host);
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    void set_connection_timeout(int seconds);
    void set_read_timeout(int seconds);
    void set_keep_alive(bool enabled);

    /// Synchronous GET, returns full response body.
    Result Get(const std::string& path);

    /// Streaming GET, invokes callback for each data chunk.
    /// Callback returns false to cancel.
    Result Get(const std::string& path, std::function<bool(const char* data, size_t len)> chunk_cb);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// -- Server-side stubs (unimplemented) ---------------------------------------
// These exist only so that engine code (RestRouter, EndpointFactory) compiles
// on iOS. The server is never instantiated in the Flutter app.

struct Request {
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> path_params;
    std::map<std::string, std::string> params;
    std::string get_header_value(const std::string& /*key*/) const {
        return {};
    }
};

using Handler = std::function<void(const Request&, Response&)>;

// Server stub — not functional on iOS. All methods log a fatal error if called.
// Exists only so engine code (RestRouter, EndpointFactory) compiles.
class Server {
  public:
    Server& Get(const std::string&, Handler) {
        fatal();
        return *this;
    }
    Server& Post(const std::string&, Handler) {
        fatal();
        return *this;
    }
    Server& Put(const std::string&, Handler) {
        fatal();
        return *this;
    }
    Server& Patch(const std::string&, Handler) {
        fatal();
        return *this;
    }
    Server& Delete(const std::string&, Handler) {
        fatal();
        return *this;
    }
    Server& Options(const std::string&, Handler) {
        fatal();
        return *this;
    }
    bool bind_to_port(const std::string&, int, int = 0) {
        fatal();
        return false;
    }
    int bind_to_any_port(const std::string&, int = 0) {
        fatal();
        return -1;
    }
    bool listen_after_bind() {
        fatal();
        return false;
    }
    void stop() {
        fatal();
    }

  private:
    [[noreturn]] static void fatal();
};

} // namespace httplib
