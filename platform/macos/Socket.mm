/**
 * @file Socket.mm
 * @brief macOS platform::Socket using Network.framework for TLS connections
 *        and BSD sockets for plain TCP (listeners, accept, non-TLS clients).
 *
 * Network.framework handles TLS natively with proper async I/O, avoiding
 * the incompatibility between Secure Transport and poll()/kqueue.
 * A wakeup pipe bridges the async receive to fd-based polling.
 */
#include "platform/Socket.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#import <Foundation/Foundation.h>
#import <Network/Network.h>
#import <Security/Security.h>

#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace platform {

// ---------------------------------------------------------------------------
// Socket::Impl — two modes: raw fd (for listeners/accept/plain TCP)
//                            or nw_connection (for TLS client connections)
// ---------------------------------------------------------------------------

struct Socket::Impl {
    // --- Raw fd path (listeners, accepted connections, plain TCP) ---
    int fd = -1;

    // --- Network.framework path (TLS client connections) ---
    nw_connection_t nw_conn = nil;
    dispatch_queue_t nw_queue = nil;
    std::string negotiated_proto;

    // Receive buffer: nw_connection_receive fills this, recv() drains it.
    std::mutex recv_mutex;
    std::condition_variable recv_cv;
    std::vector<uint8_t> recv_buf;
    size_t recv_offset = 0; // read position into recv_buf (avoids O(N) erase)
    bool recv_eof = false;
    bool recv_error = false;

    // Wakeup pipe: write end signaled when data arrives, read end
    // returned by fd() for poll/kqueue compatibility.
    int wake_pipe[2] = {-1, -1};

    // Recv timeout for blocking recv()
    int recv_timeout_ms = 0; // 0 = no timeout

    bool is_nw() const { return nw_conn != nil; }

    std::atomic<bool> closed{false};

    void schedule_receive() {
        if (!nw_conn || closed)
            return;

        auto *self = this;
        nw_connection_receive(nw_conn, 1, 65536,
                              ^(dispatch_data_t data, nw_content_context_t ctx, bool is_complete, nw_error_t error) {
                                (void)ctx;
                                if (self->closed)
                                    return;
                                bool should_continue = false;
                                {
                                    std::lock_guard lock(self->recv_mutex);

                                    if (data) {
                                        dispatch_data_apply(data, ^bool(dispatch_data_t region, size_t offset,
                                                                        const void *buf, size_t len) {
                                          (void)region;
                                          (void)offset;
                                          self->recv_buf.insert(self->recv_buf.end(), static_cast<const uint8_t *>(buf),
                                                                static_cast<const uint8_t *>(buf) + len);
                                          return true;
                                        });
                                        if (self->wake_pipe[1] >= 0) {
                                            char c = 1;
                                            ::write(self->wake_pipe[1], &c, 1);
                                        }
                                    }

                                    if (error) {
                                        self->recv_error = true;
                                    } else if (is_complete) {
                                        self->recv_eof = true;
                                    } else {
                                        should_continue = true;
                                    }

                                    if (self->recv_error || self->recv_eof) {
                                        if (self->wake_pipe[1] >= 0) {
                                            char c = 1;
                                            ::write(self->wake_pipe[1], &c, 1);
                                        }
                                    }

                                    self->recv_cv.notify_all();
                                }

                                // Schedule next receive outside the lock
                                if (should_continue)
                                    self->schedule_receive();
                              });
    }

    ~Impl() { close(); }

    void close() {
        closed = true;
        recv_cv.notify_all();
        if (nw_conn) {
            // Wait for cancellation to complete so no callbacks fire after close
            dispatch_semaphore_t cancel_sem = dispatch_semaphore_create(0);
            nw_connection_set_state_changed_handler(nw_conn, ^(nw_connection_state_t state, nw_error_t error) {
              (void)error;
              if (state == nw_connection_state_cancelled)
                  dispatch_semaphore_signal(cancel_sem);
            });
            nw_connection_cancel(nw_conn);
            dispatch_semaphore_wait(cancel_sem, dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC));
            nw_conn = nil;
        }
        nw_queue = nil;
        if (fd >= 0) {
            ::close(fd);
            fd = -1;
        }
        if (wake_pipe[0] >= 0) {
            ::close(wake_pipe[0]);
            wake_pipe[0] = -1;
        }
        if (wake_pipe[1] >= 0) {
            ::close(wake_pipe[1]);
            wake_pipe[1] = -1;
        }
    }

    void shutdown_impl() {
        if (nw_conn) {
            nw_connection_cancel(nw_conn);
        }
        if (fd >= 0) {
            ::shutdown(fd, SHUT_RDWR);
        }
        // Signal wakeup so any blocked recv() returns
        if (wake_pipe[1] >= 0) {
            char c = 1;
            ::write(wake_pipe[1], &c, 1);
        }
        recv_eof = true;
        recv_cv.notify_all();
    }
};

// ---------------------------------------------------------------------------
// Socket public interface
// ---------------------------------------------------------------------------

Socket::Socket() : impl_(std::make_unique<Impl>()) {}
Socket::~Socket() = default;
Socket::Socket(Socket &&) noexcept = default;
Socket &Socket::operator=(Socket &&) noexcept = default;
Socket::Socket(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}

bool Socket::valid() const { return impl_ && (impl_->fd >= 0 || impl_->nw_conn != nil); }

int Socket::fd() const {
    if (!impl_)
        return -1;
    // For nw_connection sockets, return the wakeup pipe read end
    if (impl_->is_nw())
        return impl_->wake_pipe[0];
    return impl_->fd;
}

bool Socket::is_ssl() const {
    return impl_ && impl_->is_nw(); // nw_connection always uses TLS
}

ssize_t Socket::send(const void *data, size_t len) {
    if (impl_->is_nw()) {
        // We must copy the data because nw_connection_send is async and the
        // caller's buffer may be invalidated before the send completes.
        void *copy = malloc(len);
        memcpy(copy, data, len);
        dispatch_data_t nw_data = dispatch_data_create(copy, len, dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^{
          free(copy);
        });

        // Use a semaphore on a DIFFERENT queue than nw_queue to avoid
        // deadlocking the serial queue (receive handlers run on nw_queue).
        dispatch_semaphore_t sem = dispatch_semaphore_create(0);
        __block bool send_ok = true;

        nw_connection_send(impl_->nw_conn, nw_data, NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT, false, ^(nw_error_t error) {
          if (error)
              send_ok = false;
          dispatch_semaphore_signal(sem);
        });

        // The completion handler runs on nw_queue. We must NOT block nw_queue.
        // Since send() is called from the h2 I/O thread (not nw_queue),
        // this semaphore wait is safe.
        dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 10 * NSEC_PER_SEC));

        return send_ok ? static_cast<ssize_t>(len) : -1;
    }
    return ::write(impl_->fd, data, len);
}

ssize_t Socket::recv(void *buf, size_t len) {
    if (impl_->is_nw()) {
        std::unique_lock lock(impl_->recv_mutex);

        // Wait for data, EOF, or error
        auto has_data = [this] {
            return impl_->recv_offset < impl_->recv_buf.size() || impl_->recv_eof || impl_->recv_error;
        };
        if (!has_data()) {
            if (impl_->recv_timeout_ms > 0)
                impl_->recv_cv.wait_for(lock, std::chrono::milliseconds(impl_->recv_timeout_ms), has_data);
            else
                impl_->recv_cv.wait(lock, has_data);
        }

        size_t avail = impl_->recv_buf.size() - impl_->recv_offset;
        if (avail > 0) {
            size_t n = std::min(len, avail);
            memcpy(buf, impl_->recv_buf.data() + impl_->recv_offset, n);
            impl_->recv_offset += n;

            // Compact buffer when fully consumed
            if (impl_->recv_offset == impl_->recv_buf.size()) {
                impl_->recv_buf.clear();
                impl_->recv_offset = 0;
                // Drain wakeup pipe (already non-blocking from pipe creation)
                if (impl_->wake_pipe[0] >= 0) {
                    char tmp[64];
                    while (::read(impl_->wake_pipe[0], tmp, sizeof(tmp)) > 0) {
                    }
                }
            }
            return static_cast<ssize_t>(n);
        }

        if (impl_->recv_eof)
            return 0;
        if (impl_->recv_error)
            return -1;
        return -1; // timeout
    }
    return ::read(impl_->fd, buf, len);
}

void Socket::set_non_blocking(bool enabled) {
    if (impl_->is_nw())
        return; // nw_connection manages its own I/O mode
    if (impl_->fd < 0)
        return;
    int flags = fcntl(impl_->fd, F_GETFL, 0);
    if (enabled)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;
    fcntl(impl_->fd, F_SETFL, flags);
}

void Socket::set_reuse_addr(bool enabled) {
    if (impl_->is_nw())
        return;
    int val = enabled ? 1 : 0;
    setsockopt(impl_->fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
}

void Socket::set_tcp_nodelay(bool enabled) {
    if (impl_->is_nw()) {
        // TCP_NODELAY is set via nw_parameters at connection creation time.
        // Can't change after the fact with Network.framework.
        return;
    }
    int val = enabled ? 1 : 0;
    setsockopt(impl_->fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
}

void Socket::set_recv_timeout(int timeout_ms) {
    if (impl_->is_nw()) {
        impl_->recv_timeout_ms = timeout_ms;
        return;
    }
    struct timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(impl_->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

void Socket::set_send_timeout(int timeout_ms) {
    if (impl_->is_nw())
        return; // nw_connection_send has its own timeout
    struct timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(impl_->fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

bool Socket::bytes_available() const {
    if (impl_->is_nw()) {
        std::lock_guard lock(impl_->recv_mutex);
        return impl_->recv_offset < impl_->recv_buf.size();
    }
    int count = 0;
    ioctl(impl_->fd, FIONREAD, &count);
    return count > 0;
}

uint16_t Socket::local_port() const {
    if (impl_->is_nw())
        return 0; // nw_connection doesn't easily expose this
    if (!impl_ || impl_->fd < 0)
        return 0;
    struct sockaddr_storage ss{};
    socklen_t len = sizeof(ss);
    if (getsockname(impl_->fd, reinterpret_cast<sockaddr *>(&ss), &len) == 0) {
        if (ss.ss_family == AF_INET)
            return ntohs(reinterpret_cast<sockaddr_in *>(&ss)->sin_port);
        if (ss.ss_family == AF_INET6)
            return ntohs(reinterpret_cast<sockaddr_in6 *>(&ss)->sin6_port);
    }
    return 0;
}

void Socket::shutdown() {
    if (impl_)
        impl_->shutdown_impl();
}

void Socket::close() {
    if (impl_)
        impl_->close();
}

void Socket::ssl_connect(const std::string &hostname, const std::string &alpn_protos) {
    // Upgrade this socket from a raw fd to an nw_connection with TLS.
    // This replaces the fd with a Network.framework connection.
    if (impl_->fd < 0)
        throw std::runtime_error("ssl_connect: socket not connected");

    // Get the remote address from the existing fd
    struct sockaddr_storage sa{};
    socklen_t sa_len = sizeof(sa);
    if (getpeername(impl_->fd, reinterpret_cast<sockaddr *>(&sa), &sa_len) < 0)
        throw std::runtime_error("ssl_connect: getpeername failed");

    uint16_t port = 0;
    char addr_buf[INET6_ADDRSTRLEN] = {};
    if (sa.ss_family == AF_INET) {
        auto *s4 = reinterpret_cast<sockaddr_in *>(&sa);
        inet_ntop(AF_INET, &s4->sin_addr, addr_buf, sizeof(addr_buf));
        port = ntohs(s4->sin_port);
    } else {
        auto *s6 = reinterpret_cast<sockaddr_in6 *>(&sa);
        inet_ntop(AF_INET6, &s6->sin6_addr, addr_buf, sizeof(addr_buf));
        port = ntohs(s6->sin6_port);
    }

    // Close the raw fd — we'll create a new nw_connection
    ::close(impl_->fd);
    impl_->fd = -1;

    // Create wakeup pipe
    if (::pipe(impl_->wake_pipe) < 0)
        throw std::runtime_error("ssl_connect: pipe() failed");
    // Make read end non-blocking for drain operations
    fcntl(impl_->wake_pipe[0], F_SETFL, O_NONBLOCK);

    // Configure TLS
    nw_parameters_t params = nw_parameters_create_secure_tcp(
        ^(nw_protocol_options_t tls_options) {
          sec_protocol_options_t sec_opts = nw_tls_copy_sec_protocol_options(tls_options);
          sec_protocol_options_set_tls_server_name(sec_opts, hostname.c_str());

          // ALPN
          if (!alpn_protos.empty()) {
              std::string remaining = alpn_protos;
              while (!remaining.empty()) {
                  auto comma = remaining.find(',');
                  std::string proto = (comma != std::string::npos) ? remaining.substr(0, comma) : remaining;
                  remaining = (comma != std::string::npos) ? remaining.substr(comma + 1) : "";
                  if (!proto.empty())
                      sec_protocol_options_add_tls_application_protocol(sec_opts, proto.c_str());
              }
          }
        },
        ^(nw_protocol_options_t tcp_options) {
          nw_tcp_options_set_no_delay(tcp_options, true);
        });

    // Create endpoint and connection
    std::string port_str = std::to_string(port);
    nw_endpoint_t endpoint = nw_endpoint_create_host(hostname.c_str(), port_str.c_str());
    nw_connection_t conn = nw_connection_create(endpoint, params);

    impl_->nw_queue = dispatch_queue_create("platform.socket.nw", DISPATCH_QUEUE_SERIAL);
    impl_->nw_conn = conn;

    // Wait for connection ready
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    __block bool connected = false;
    __block std::string error_msg;

    nw_connection_set_state_changed_handler(conn, ^(nw_connection_state_t state, nw_error_t error) {
      if (state == nw_connection_state_ready) {
          connected = true;
          dispatch_semaphore_signal(sem);
      } else if (state == nw_connection_state_failed || state == nw_connection_state_cancelled) {
          if (error) {
              error_msg =
                  [NSString stringWithFormat:@"%@", (__bridge NSError *)nw_error_copy_cf_error(error)].UTF8String;
          } else {
              error_msg = "connection failed";
          }
          dispatch_semaphore_signal(sem);
      }
    });

    nw_connection_set_queue(conn, impl_->nw_queue);
    nw_connection_start(conn);

    // Wait with timeout
    auto timeout = dispatch_time(DISPATCH_TIME_NOW, 10 * NSEC_PER_SEC);
    if (dispatch_semaphore_wait(sem, timeout) != 0) {
        impl_->close();
        throw std::runtime_error("ssl_connect: connection timeout to " + hostname);
    }

    if (!connected) {
        impl_->close();
        throw std::runtime_error("ssl_connect: " + error_msg);
    }

    // Read negotiated ALPN
    nw_protocol_metadata_t tls_meta = nw_connection_copy_protocol_metadata(conn, nw_protocol_copy_tls_definition());
    if (tls_meta) {
        sec_protocol_metadata_t sec_meta = nw_tls_copy_sec_protocol_metadata(tls_meta);
        if (sec_meta) {
            const char *proto = sec_protocol_metadata_get_negotiated_protocol(sec_meta);
            if (proto)
                impl_->negotiated_proto = proto;
        }
    }

    // Start continuous receive loop
    impl_->schedule_receive();
}

std::string Socket::negotiated_protocol() const { return impl_ ? impl_->negotiated_proto : ""; }

void Socket::ssl_accept() { throw std::runtime_error("ssl_accept: not implemented with Network.framework"); }

// ---------------------------------------------------------------------------
// Factory functions (use raw BSD sockets — no TLS)
// ---------------------------------------------------------------------------

Socket tcp_create() {
    auto impl = std::make_unique<Socket::Impl>();
    impl->fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (impl->fd < 0)
        throw std::runtime_error("socket() failed");
    return Socket(std::move(impl));
}

Socket tcp_connect(const std::string &host, uint16_t port, int timeout_ms) {
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string port_str = std::to_string(port);
    int err = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res);
    if (err != 0 || !res)
        throw std::runtime_error("getaddrinfo failed for " + host + ":" + port_str + ": " + gai_strerror(err));

    auto impl = std::make_unique<Socket::Impl>();
    for (auto *rp = res; rp; rp = rp->ai_next) {
        impl->fd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (impl->fd < 0)
            continue;

        if (timeout_ms >= 0) {
            int flags = fcntl(impl->fd, F_GETFL, 0);
            fcntl(impl->fd, F_SETFL, flags | O_NONBLOCK);

            int ret = ::connect(impl->fd, rp->ai_addr, rp->ai_addrlen);
            if (ret < 0 && errno != EINPROGRESS) {
                ::close(impl->fd);
                impl->fd = -1;
                continue;
            }
            if (ret != 0) {
                struct pollfd pfd{};
                pfd.fd = impl->fd;
                pfd.events = POLLOUT;
                int poll_ret = ::poll(&pfd, 1, timeout_ms);
                if (poll_ret <= 0) {
                    ::close(impl->fd);
                    impl->fd = -1;
                    continue;
                }
                int so_error = 0;
                socklen_t so_len = sizeof(so_error);
                getsockopt(impl->fd, SOL_SOCKET, SO_ERROR, &so_error, &so_len);
                if (so_error != 0) {
                    ::close(impl->fd);
                    impl->fd = -1;
                    continue;
                }
            }
            fcntl(impl->fd, F_SETFL, flags);
        } else {
            if (::connect(impl->fd, rp->ai_addr, rp->ai_addrlen) != 0) {
                ::close(impl->fd);
                impl->fd = -1;
                continue;
            }
        }
        break;
    }
    freeaddrinfo(res);

    if (impl->fd < 0)
        throw std::runtime_error("tcp_connect failed to " + host + ":" + port_str);
    return Socket(std::move(impl));
}

Socket tcp_listen(const std::string &addr, uint16_t port, int backlog) {
    auto impl = std::make_unique<Socket::Impl>();
    impl->fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (impl->fd < 0)
        throw std::runtime_error("socket() failed");

    int reuse = 1;
    setsockopt(impl->fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (addr.empty() || addr == "0.0.0.0")
        sa.sin_addr.s_addr = INADDR_ANY;
    else
        inet_pton(AF_INET, addr.c_str(), &sa.sin_addr);

    if (::bind(impl->fd, reinterpret_cast<sockaddr *>(&sa), sizeof(sa)) < 0)
        throw std::runtime_error("bind() failed on " + addr + ":" + std::to_string(port));
    if (::listen(impl->fd, backlog) < 0)
        throw std::runtime_error("listen() failed");
    return Socket(std::move(impl));
}

Socket tcp_accept(Socket &listener, std::string &remote_addr, uint16_t &remote_port) {
    struct sockaddr_storage sa{};
    socklen_t sa_len = sizeof(sa);
    int client_fd = ::accept(listener.fd(), reinterpret_cast<sockaddr *>(&sa), &sa_len);
    if (client_fd < 0)
        return Socket();

    char addr_buf[INET6_ADDRSTRLEN] = {};
    if (sa.ss_family == AF_INET) {
        auto *s4 = reinterpret_cast<sockaddr_in *>(&sa);
        inet_ntop(AF_INET, &s4->sin_addr, addr_buf, sizeof(addr_buf));
        remote_port = ntohs(s4->sin_port);
    } else if (sa.ss_family == AF_INET6) {
        auto *s6 = reinterpret_cast<sockaddr_in6 *>(&sa);
        inet_ntop(AF_INET6, &s6->sin6_addr, addr_buf, sizeof(addr_buf));
        remote_port = ntohs(s6->sin6_port);
    }
    remote_addr = addr_buf;

    auto impl = std::make_unique<Socket::Impl>();
    impl->fd = client_fd;
    return Socket(std::move(impl));
}

// ---------------------------------------------------------------------------
// Global TLS server context (stub — server TLS not needed with NW.framework)
// ---------------------------------------------------------------------------

void ssl_init_server(const std::string &, const std::string &) {
    // Server-side TLS via Network.framework would use nw_listener with
    // sec_protocol_options. Not implemented — server runs plain TCP.
}

} // namespace platform
