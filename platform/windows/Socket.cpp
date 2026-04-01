#include "platform/Socket.h"

#ifndef _WIN32
#error "This file should only be compiled on Windows"
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <mutex>
#include <stdexcept>

#pragma comment(lib, "ws2_32.lib")

namespace platform {

// ---------------------------------------------------------------------------
// WinSock initialisation (RAII)
// ---------------------------------------------------------------------------

static struct WinsockInit {
    WinsockInit() {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
    }
    ~WinsockInit() {
        WSACleanup();
    }
} g_winsock_init;

// ---------------------------------------------------------------------------
// Global OpenSSL state
// ---------------------------------------------------------------------------

static std::once_flag g_ssl_init_flag;
static SSL_CTX* g_client_ctx = nullptr;
static SSL_CTX* g_server_ctx = nullptr;

static void ensure_openssl() {
    std::call_once(g_ssl_init_flag, [] {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        g_client_ctx = SSL_CTX_new(TLS_client_method());
        SSL_CTX_set_default_verify_paths(g_client_ctx);
        SSL_CTX_set_verify(g_client_ctx, SSL_VERIFY_PEER, nullptr);
    });
}

// ---------------------------------------------------------------------------
// Socket::Impl — uses SOCKET (unsigned) on Windows
// ---------------------------------------------------------------------------

struct Socket::Impl {
    SOCKET winsock = INVALID_SOCKET;
    SSL* ssl = nullptr;

    int to_fd() const {
        return (winsock == INVALID_SOCKET) ? -1 : static_cast<int>(winsock);
    }

    ~Impl() {
        close();
    }

    void close() {
        if (ssl) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            ssl = nullptr;
        }
        if (winsock != INVALID_SOCKET) {
            closesocket(winsock);
            winsock = INVALID_SOCKET;
        }
    }

    void shutdown() {
        if (ssl) {
            SSL_shutdown(ssl);
        }
        if (winsock != INVALID_SOCKET) {
            ::shutdown(winsock, SD_BOTH);
        }
    }
};

// ---------------------------------------------------------------------------
// Socket public interface
// ---------------------------------------------------------------------------

Socket::Socket() : impl_(std::make_unique<Impl>()) {
}
Socket::~Socket() = default;
Socket::Socket(Socket&&) noexcept = default;
Socket& Socket::operator=(Socket&&) noexcept = default;
Socket::Socket(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {
}

bool Socket::valid() const {
    return impl_ && impl_->winsock != INVALID_SOCKET;
}
int Socket::fd() const {
    return impl_ ? impl_->to_fd() : -1;
}
bool Socket::is_ssl() const {
    return impl_ && impl_->ssl != nullptr;
}

ssize_t Socket::send(const void* data, size_t len) {
    if (impl_->ssl) {
        int n = SSL_write(impl_->ssl, data, static_cast<int>(len));
        return (n > 0) ? n : -1;
    }
    return ::send(impl_->winsock, static_cast<const char*>(data), static_cast<int>(len), 0);
}

ssize_t Socket::recv(void* buf, size_t len) {
    if (impl_->ssl) {
        int n = SSL_read(impl_->ssl, buf, static_cast<int>(len));
        if (n > 0)
            return n;
        int err = SSL_get_error(impl_->ssl, n);
        if (err == SSL_ERROR_ZERO_RETURN)
            return 0;
        return -1;
    }
    return ::recv(impl_->winsock, static_cast<char*>(buf), static_cast<int>(len), 0);
}

void Socket::set_non_blocking(bool enabled) {
    u_long mode = enabled ? 1 : 0;
    ioctlsocket(impl_->winsock, FIONBIO, &mode);
}

void Socket::set_reuse_addr(bool enabled) {
    int val = enabled ? 1 : 0;
    setsockopt(impl_->winsock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&val), sizeof(val));
}

void Socket::set_tcp_nodelay(bool enabled) {
    int val = enabled ? 1 : 0;
    setsockopt(impl_->winsock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&val), sizeof(val));
}

void Socket::set_recv_timeout(int timeout_ms) {
    DWORD tv = static_cast<DWORD>(timeout_ms);
    setsockopt(impl_->winsock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
}

void Socket::set_send_timeout(int timeout_ms) {
    DWORD tv = static_cast<DWORD>(timeout_ms);
    setsockopt(impl_->winsock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
}

bool Socket::bytes_available() const {
    u_long count = 0;
    ioctlsocket(impl_->winsock, FIONREAD, &count);
    return count > 0;
}

uint16_t Socket::local_port() const {
    if (!impl_ || impl_->winsock == INVALID_SOCKET)
        return 0;
    struct sockaddr_in sa{};
    int len = sizeof(sa);
    if (getsockname(impl_->winsock, reinterpret_cast<sockaddr*>(&sa), &len) == 0) {
        return ntohs(sa.sin_port);
    }
    return 0;
}

void Socket::shutdown() {
    if (impl_)
        impl_->shutdown();
}

void Socket::close() {
    if (impl_)
        impl_->close();
}

void Socket::ssl_connect(const std::string& hostname) {
    ensure_openssl();
    SSL* ssl = SSL_new(g_client_ctx);
    if (!ssl)
        throw std::runtime_error("SSL_new failed");

    SSL_set_fd(ssl, static_cast<int>(impl_->winsock));
    SSL_set_tlsext_host_name(ssl, hostname.c_str());
    SSL_set1_host(ssl, hostname.c_str());

    int ret = SSL_connect(ssl);
    if (ret != 1) {
        unsigned long err = ERR_get_error();
        char err_buf[256];
        ERR_error_string_n(err, err_buf, sizeof(err_buf));
        SSL_free(ssl);
        throw std::runtime_error("SSL_connect failed: " + std::string(err_buf));
    }
    impl_->ssl = ssl;
}

void Socket::ssl_accept() {
    if (!g_server_ctx) {
        throw std::runtime_error("ssl_accept: ssl_init_server() has not been called");
    }
    SSL* ssl = SSL_new(g_server_ctx);
    if (!ssl)
        throw std::runtime_error("SSL_new failed (server)");

    SSL_set_fd(ssl, static_cast<int>(impl_->winsock));
    int ret = SSL_accept(ssl);
    if (ret != 1) {
        unsigned long err = ERR_get_error();
        char err_buf[256];
        ERR_error_string_n(err, err_buf, sizeof(err_buf));
        SSL_free(ssl);
        throw std::runtime_error("SSL_accept failed: " + std::string(err_buf));
    }
    impl_->ssl = ssl;
}

// ---------------------------------------------------------------------------
// Factory functions
// ---------------------------------------------------------------------------

Socket tcp_create() {
    auto impl = std::make_unique<Socket::Impl>();
    impl->winsock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (impl->winsock == INVALID_SOCKET)
        throw std::runtime_error("socket() failed");
    return Socket(std::move(impl));
}

Socket tcp_connect(const std::string& host, uint16_t port, int timeout_ms) {
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string port_str = std::to_string(port);
    int err = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res);
    if (err != 0 || !res) {
        throw std::runtime_error("getaddrinfo failed for " + host + ":" + port_str);
    }

    auto impl = std::make_unique<Socket::Impl>();
    for (auto* rp = res; rp; rp = rp->ai_next) {
        impl->winsock = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (impl->winsock == INVALID_SOCKET)
            continue;

        if (timeout_ms >= 0) {
            // Non-blocking connect with timeout
            u_long mode = 1;
            ioctlsocket(impl->winsock, FIONBIO, &mode);

            int ret = ::connect(impl->winsock, rp->ai_addr, static_cast<int>(rp->ai_addrlen));
            if (ret == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
                closesocket(impl->winsock);
                impl->winsock = INVALID_SOCKET;
                continue;
            }
            if (ret == SOCKET_ERROR) {
                WSAPOLLFD pfd{};
                pfd.fd = impl->winsock;
                pfd.events = POLLWRNORM;
                int poll_ret = WSAPoll(&pfd, 1, timeout_ms);
                if (poll_ret <= 0) {
                    closesocket(impl->winsock);
                    impl->winsock = INVALID_SOCKET;
                    continue;
                }
                int so_error = 0;
                int so_len = sizeof(so_error);
                getsockopt(impl->winsock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&so_error), &so_len);
                if (so_error != 0) {
                    closesocket(impl->winsock);
                    impl->winsock = INVALID_SOCKET;
                    continue;
                }
            }

            // Restore blocking mode
            mode = 0;
            ioctlsocket(impl->winsock, FIONBIO, &mode);
        }
        else {
            if (::connect(impl->winsock, rp->ai_addr, static_cast<int>(rp->ai_addrlen)) != 0) {
                closesocket(impl->winsock);
                impl->winsock = INVALID_SOCKET;
                continue;
            }
        }
        break;
    }
    freeaddrinfo(res);

    if (impl->winsock == INVALID_SOCKET) {
        throw std::runtime_error("tcp_connect failed to " + host + ":" + port_str);
    }
    return Socket(std::move(impl));
}

Socket tcp_listen(const std::string& addr, uint16_t port, int backlog) {
    auto impl = std::make_unique<Socket::Impl>();
    impl->winsock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (impl->winsock == INVALID_SOCKET)
        throw std::runtime_error("socket() failed");

    int reuse = 1;
    setsockopt(impl->winsock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    struct sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (addr.empty() || addr == "0.0.0.0") {
        sa.sin_addr.s_addr = INADDR_ANY;
    }
    else {
        inet_pton(AF_INET, addr.c_str(), &sa.sin_addr);
    }

    if (::bind(impl->winsock, reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) == SOCKET_ERROR) {
        throw std::runtime_error("bind() failed on " + addr + ":" + std::to_string(port));
    }
    if (::listen(impl->winsock, backlog) == SOCKET_ERROR) {
        throw std::runtime_error("listen() failed");
    }
    return Socket(std::move(impl));
}

Socket tcp_accept(Socket& listener, std::string& remote_addr, uint16_t& remote_port) {
    struct sockaddr_storage sa{};
    int sa_len = sizeof(sa);
    SOCKET client = ::accept(static_cast<SOCKET>(listener.fd()), reinterpret_cast<sockaddr*>(&sa), &sa_len);
    if (client == INVALID_SOCKET) {
        return Socket();
    }

    char addr_buf[INET6_ADDRSTRLEN] = {};
    if (sa.ss_family == AF_INET) {
        auto* s4 = reinterpret_cast<sockaddr_in*>(&sa);
        inet_ntop(AF_INET, &s4->sin_addr, addr_buf, sizeof(addr_buf));
        remote_port = ntohs(s4->sin_port);
    }
    else if (sa.ss_family == AF_INET6) {
        auto* s6 = reinterpret_cast<sockaddr_in6*>(&sa);
        inet_ntop(AF_INET6, &s6->sin6_addr, addr_buf, sizeof(addr_buf));
        remote_port = ntohs(s6->sin6_port);
    }
    remote_addr = addr_buf;

    auto impl = std::make_unique<Socket::Impl>();
    impl->winsock = client;
    return Socket(std::move(impl));
}

// ---------------------------------------------------------------------------
// Global TLS server context
// ---------------------------------------------------------------------------

void ssl_init_server(const std::string& cert_path, const std::string& key_path) {
    ensure_openssl();
    g_server_ctx = SSL_CTX_new(TLS_server_method());
    if (!g_server_ctx)
        throw std::runtime_error("SSL_CTX_new failed (server)");

    if (SSL_CTX_use_certificate_chain_file(g_server_ctx, cert_path.c_str()) != 1) {
        throw std::runtime_error("ssl_init_server: failed to load certificate from " + cert_path);
    }
    if (SSL_CTX_use_PrivateKey_file(g_server_ctx, key_path.c_str(), SSL_FILETYPE_PEM) != 1) {
        throw std::runtime_error("ssl_init_server: failed to load private key from " + key_path);
    }
    if (SSL_CTX_check_private_key(g_server_ctx) != 1) {
        throw std::runtime_error("ssl_init_server: certificate and key do not match");
    }
}

} // namespace platform
