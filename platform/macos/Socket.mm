#include "platform/Socket.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#import <Foundation/Foundation.h>
#import <Security/SecureTransport.h>
#import <Security/Security.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include <stdexcept>

namespace platform {

// ---------------------------------------------------------------------------
// Global server TLS identity (loaded once via ssl_init_server)
// ---------------------------------------------------------------------------

static SecIdentityRef g_server_identity = nullptr;

// ---------------------------------------------------------------------------
// Socket::Impl
// ---------------------------------------------------------------------------

struct Socket::Impl {
    int fd = -1;
    SSLContextRef ssl_ctx = nullptr;

    ~Impl() { close(); }

    void close() {
        if (ssl_ctx) {
            SSLClose(ssl_ctx);
            CFRelease(ssl_ctx);
            ssl_ctx = nullptr;
        }
        if (fd >= 0) {
            ::close(fd);
            fd = -1;
        }
    }

    void shutdown() {
        if (ssl_ctx) {
            SSLClose(ssl_ctx);
        }
        if (fd >= 0) {
            ::shutdown(fd, SHUT_RDWR);
        }
    }
};

// ---------------------------------------------------------------------------
// Secure Transport I/O callbacks
// ---------------------------------------------------------------------------

static OSStatus ssl_read_cb(SSLConnectionRef conn, void *data, size_t *len) {
    int fd = static_cast<int>(reinterpret_cast<intptr_t>(conn));
    size_t requested = *len;
    size_t total = 0;
    auto *buf = static_cast<uint8_t *>(data);

    while (total < requested) {
        ssize_t n = ::read(fd, buf + total, requested - total);
        if (n > 0) {
            total += static_cast<size_t>(n);
        } else if (n == 0) {
            *len = total;
            return errSSLClosedGraceful;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                *len = total;
                return (total > 0) ? noErr : errSSLWouldBlock;
            }
            *len = total;
            return errSecIO;
        }
    }
    *len = total;
    return noErr;
}

static OSStatus ssl_write_cb(SSLConnectionRef conn, const void *data, size_t *len) {
    int fd = static_cast<int>(reinterpret_cast<intptr_t>(conn));
    size_t requested = *len;
    size_t total = 0;
    auto *buf = static_cast<const uint8_t *>(data);

    while (total < requested) {
        ssize_t n = ::write(fd, buf + total, requested - total);
        if (n > 0) {
            total += static_cast<size_t>(n);
        } else if (n == 0) {
            *len = total;
            return errSSLClosedGraceful;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                *len = total;
                return (total > 0) ? noErr : errSSLWouldBlock;
            }
            *len = total;
            return errSecIO;
        }
    }
    *len = total;
    return noErr;
}

// ---------------------------------------------------------------------------
// Socket public interface
// ---------------------------------------------------------------------------

Socket::Socket() : impl_(std::make_unique<Impl>()) {}
Socket::~Socket() = default;
Socket::Socket(Socket &&) noexcept = default;
Socket &Socket::operator=(Socket &&) noexcept = default;
Socket::Socket(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}

bool Socket::valid() const { return impl_ && impl_->fd >= 0; }
int Socket::fd() const { return impl_ ? impl_->fd : -1; }
bool Socket::is_ssl() const { return impl_ && impl_->ssl_ctx != nullptr; }

ssize_t Socket::send(const void *data, size_t len) {
    if (impl_->ssl_ctx) {
        size_t written = 0;
        OSStatus st = SSLWrite(impl_->ssl_ctx, data, len, &written);
        if (st == noErr || st == errSSLWouldBlock)
            return static_cast<ssize_t>(written);
        return -1;
    }
    return ::write(impl_->fd, data, len);
}

ssize_t Socket::recv(void *buf, size_t len) {
    if (impl_->ssl_ctx) {
        size_t read_bytes = 0;
        OSStatus st = SSLRead(impl_->ssl_ctx, buf, len, &read_bytes);
        if (st == noErr || st == errSSLWouldBlock)
            return static_cast<ssize_t>(read_bytes);
        if (st == errSSLClosedGraceful || st == errSSLClosedNoNotify)
            return 0;
        return -1;
    }
    return ::read(impl_->fd, buf, len);
}

void Socket::set_non_blocking(bool enabled) {
    int flags = fcntl(impl_->fd, F_GETFL, 0);
    if (enabled)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;
    fcntl(impl_->fd, F_SETFL, flags);
}

void Socket::set_reuse_addr(bool enabled) {
    int val = enabled ? 1 : 0;
    setsockopt(impl_->fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
}

void Socket::set_tcp_nodelay(bool enabled) {
    int val = enabled ? 1 : 0;
    setsockopt(impl_->fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
}

bool Socket::bytes_available() const {
    int count = 0;
    ioctl(impl_->fd, FIONREAD, &count);
    return count > 0;
}

uint16_t Socket::local_port() const {
    if (!impl_ || impl_->fd < 0)
        return 0;
    struct sockaddr_in sa{};
    socklen_t len = sizeof(sa);
    if (getsockname(impl_->fd, reinterpret_cast<sockaddr *>(&sa), &len) == 0) {
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

void Socket::ssl_connect(const std::string &hostname) {
    SSLContextRef ctx = SSLCreateContext(nullptr, kSSLClientSide, kSSLStreamType);
    if (!ctx)
        throw std::runtime_error("SSLCreateContext failed");

    SSLSetIOFuncs(ctx, ssl_read_cb, ssl_write_cb);
    SSLSetConnection(ctx, reinterpret_cast<SSLConnectionRef>(static_cast<intptr_t>(impl_->fd)));
    SSLSetPeerDomainName(ctx, hostname.c_str(), hostname.size());

    OSStatus st;
    do {
        st = SSLHandshake(ctx);
    } while (st == errSSLWouldBlock);

    if (st != noErr) {
        CFRelease(ctx);
        throw std::runtime_error("SSL handshake failed (Secure Transport error " + std::to_string(st) + ")");
    }
    impl_->ssl_ctx = ctx;
}

void Socket::ssl_accept() {
    if (!g_server_identity) {
        throw std::runtime_error("ssl_accept: ssl_init_server() has not been called");
    }

    SSLContextRef ctx = SSLCreateContext(nullptr, kSSLServerSide, kSSLStreamType);
    if (!ctx)
        throw std::runtime_error("SSLCreateContext failed (server)");

    SSLSetIOFuncs(ctx, ssl_read_cb, ssl_write_cb);
    SSLSetConnection(ctx, reinterpret_cast<SSLConnectionRef>(static_cast<intptr_t>(impl_->fd)));

    CFArrayRef certs = CFArrayCreate(nullptr, (const void **)&g_server_identity, 1, &kCFTypeArrayCallBacks);
    SSLSetCertificate(ctx, certs);
    CFRelease(certs);

    OSStatus st;
    do {
        st = SSLHandshake(ctx);
    } while (st == errSSLWouldBlock);

    if (st != noErr) {
        CFRelease(ctx);
        throw std::runtime_error("SSL accept handshake failed (Secure Transport error " + std::to_string(st) + ")");
    }
    impl_->ssl_ctx = ctx;
}

// ---------------------------------------------------------------------------
// Factory functions
// ---------------------------------------------------------------------------

Socket tcp_create() {
    auto impl = std::make_unique<Socket::Impl>();
    impl->fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (impl->fd < 0)
        throw std::runtime_error("socket() failed");
    return Socket(std::move(impl));
}

Socket tcp_connect(const std::string &host, uint16_t port) {
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string port_str = std::to_string(port);
    int err = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res);
    if (err != 0 || !res) {
        throw std::runtime_error("getaddrinfo failed for " + host + ":" + port_str + ": " + gai_strerror(err));
    }

    auto impl = std::make_unique<Socket::Impl>();
    for (auto *rp = res; rp; rp = rp->ai_next) {
        impl->fd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (impl->fd < 0)
            continue;
        if (::connect(impl->fd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
        ::close(impl->fd);
        impl->fd = -1;
    }
    freeaddrinfo(res);

    if (impl->fd < 0) {
        throw std::runtime_error("tcp_connect failed to " + host + ":" + port_str);
    }
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
    if (addr.empty() || addr == "0.0.0.0") {
        sa.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, addr.c_str(), &sa.sin_addr);
    }

    if (::bind(impl->fd, reinterpret_cast<sockaddr *>(&sa), sizeof(sa)) < 0) {
        throw std::runtime_error("bind() failed on " + addr + ":" + std::to_string(port));
    }
    if (::listen(impl->fd, backlog) < 0) {
        throw std::runtime_error("listen() failed");
    }
    return Socket(std::move(impl));
}

Socket tcp_accept(Socket &listener, std::string &remote_addr, uint16_t &remote_port) {
    struct sockaddr_storage sa{};
    socklen_t sa_len = sizeof(sa);
    int client_fd = ::accept(listener.fd(), reinterpret_cast<sockaddr *>(&sa), &sa_len);
    if (client_fd < 0) {
        return Socket(); // invalid — non-blocking with no pending connection, or error
    }

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
// Global TLS server context
// ---------------------------------------------------------------------------

void ssl_init_server(const std::string &cert_path, const std::string &key_path) {
    // Load PKCS#12 or PEM certificate + key into a SecIdentity.
    // For simplicity, this expects a PKCS#12 (.p12) file at cert_path.
    // key_path is used as the passphrase (empty string for no passphrase).
    @autoreleasepool {
        NSData *p12_data = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:cert_path.c_str()]];
        if (!p12_data) {
            throw std::runtime_error("ssl_init_server: could not read " + cert_path);
        }

        NSDictionary *options =
            @{(__bridge NSString *)kSecImportExportPassphrase : [NSString stringWithUTF8String:key_path.c_str()]};

        CFArrayRef items = nullptr;
        OSStatus st = SecPKCS12Import((__bridge CFDataRef)p12_data, (__bridge CFDictionaryRef)options, &items);
        if (st != errSecSuccess || !items || CFArrayGetCount(items) == 0) {
            throw std::runtime_error("ssl_init_server: SecPKCS12Import failed (error " + std::to_string(st) + ")");
        }

        CFDictionaryRef dict = (CFDictionaryRef)CFArrayGetValueAtIndex(items, 0);
        SecIdentityRef identity = (SecIdentityRef)CFDictionaryGetValue(dict, kSecImportItemIdentity);
        if (!identity) {
            CFRelease(items);
            throw std::runtime_error("ssl_init_server: no identity in PKCS#12");
        }

        CFRetain(identity);
        g_server_identity = identity;
        CFRelease(items);
    }
}

} // namespace platform

#pragma clang diagnostic pop
