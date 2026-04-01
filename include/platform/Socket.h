#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#ifdef _WIN32
using ssize_t = long long;
#endif

namespace platform {

/// Opaque socket handle wrapping an OS file descriptor and optional TLS session.
/// Move-only. The socket is closed automatically on destruction.
///
/// I/O methods (send/recv) transparently pass through TLS when enabled.
/// Use ssl_connect() or ssl_accept() to upgrade a plain TCP connection.
///
/// Implemented per-platform in platform/{macos,linux,windows}.
class Socket {
  public:
    Socket();
    ~Socket();
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    /// True if this socket holds a valid file descriptor.
    bool valid() const;

    /// Raw file descriptor, for use with Poller. Returns -1 if invalid.
    int fd() const;

    // -- I/O (TLS-transparent) -----------------------------------------------

    /// Send bytes. Returns number of bytes sent, or -1 on error.
    ssize_t send(const void* data, size_t len);

    /// Receive bytes. Returns number of bytes read, 0 on clean disconnect,
    /// or -1 on error / would-block.
    ssize_t recv(void* buf, size_t len);

    // -- Socket options ------------------------------------------------------

    void set_non_blocking(bool enabled);
    void set_reuse_addr(bool enabled);
    void set_tcp_nodelay(bool enabled);
    void set_recv_timeout(int timeout_ms);
    void set_send_timeout(int timeout_ms);

    /// True if at least one byte can be read without blocking.
    bool bytes_available() const;

    /// Get the local port number this socket is bound to.
    /// Returns 0 if the socket is not bound or invalid.
    uint16_t local_port() const;

    // -- Lifecycle -----------------------------------------------------------

    /// Graceful shutdown (sends TCP FIN / TLS close_notify).
    void shutdown();

    /// Close the socket immediately. Called automatically by destructor.
    void close();

    // -- TLS -----------------------------------------------------------------

    /// Perform a client-side TLS handshake on this connected socket.
    /// @param hostname Used for SNI and certificate verification.
    void ssl_connect(const std::string& hostname);

    /// Perform a server-side TLS handshake using the global server context
    /// initialised by ssl_init_server().
    void ssl_accept();

    /// True if TLS has been established on this socket.
    bool is_ssl() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    /// Private: construct from an existing implementation (used by free fns).
    explicit Socket(std::unique_ptr<Impl> impl);
    friend Socket tcp_create();
    friend Socket tcp_connect(const std::string& host, uint16_t port, int timeout_ms);
    friend Socket tcp_listen(const std::string& addr, uint16_t port, int backlog);
    friend Socket tcp_accept(Socket& listener, std::string& remote_addr, uint16_t& remote_port);
};

// -- Factory functions -------------------------------------------------------

/// Create an unconnected TCP socket.
Socket tcp_create();

/// Resolve host, connect, and return the connected socket.
/// @param timeout_ms  Connection timeout in milliseconds. -1 (default) = no timeout (blocking).
/// Throws std::runtime_error on failure or timeout.
Socket tcp_connect(const std::string& host, uint16_t port, int timeout_ms = -1);

/// Bind to addr:port, start listening, and return the listener socket.
/// Throws std::runtime_error on failure.
Socket tcp_listen(const std::string& addr, uint16_t port, int backlog = 128);

/// Accept a connection from a listener socket.
/// Populates remote_addr and remote_port.
/// Returns an invalid socket if non-blocking and no connection is pending.
Socket tcp_accept(Socket& listener, std::string& remote_addr, uint16_t& remote_port);

// -- Global TLS setup --------------------------------------------------------

/// Initialise the global TLS server context with a certificate and private key.
/// Must be called before any Socket::ssl_accept() calls.
/// On macOS/iOS this loads into Secure Transport; on Linux/Windows into OpenSSL.
void ssl_init_server(const std::string& cert_path, const std::string& key_path);

} // namespace platform
