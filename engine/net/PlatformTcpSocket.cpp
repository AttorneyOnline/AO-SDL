#include "net/PlatformTcpSocket.h"

#include <cerrno>
#include <cstring>

PlatformTcpSocket::PlatformTcpSocket(const std::string& host, uint16_t port) : host_(host), port_(port) {
}

PlatformTcpSocket::PlatformTcpSocket(platform::Socket&& connected_sock) : port_(0), sock_(std::move(connected_sock)) {
}

int PlatformTcpSocket::fd() const {
    return sock_.fd();
}

void PlatformTcpSocket::enable_ssl(const std::string& hostname) {
    ssl_hostname_ = hostname;
}

void PlatformTcpSocket::set_connect_timeout(int timeout_ms) {
    connect_timeout_ms_ = timeout_ms;
}

void PlatformTcpSocket::connect() {
    sock_ = platform::tcp_connect(host_, port_, connect_timeout_ms_);
    if (!ssl_hostname_.empty()) {
        sock_.ssl_connect(ssl_hostname_);
    }
}

void PlatformTcpSocket::set_non_blocking(bool non_blocking) {
    sock_.set_non_blocking(non_blocking);
}

void PlatformTcpSocket::send(const uint8_t* data, size_t size) {
    size_t total = 0;
    while (total < size) {
        ssize_t n = sock_.send(data + total, size - total);
        if (n <= 0) {
            throw WebSocketException("TCP socket write error");
        }
        total += static_cast<size_t>(n);
    }
}

std::vector<uint8_t> PlatformTcpSocket::recv() {
    uint8_t buf[RECV_BUF_SIZE];
    ssize_t n;
    do {
        n = sock_.recv(buf, sizeof(buf));
    } while (n < 0 && errno == EINTR);

    if (n < 0) {
        // Would-block in non-blocking mode: return empty
        return {};
    }
    if (n == 0) {
        throw WebSocketException("TCP connection closed by peer");
    }

    return std::vector<uint8_t>(buf, buf + n);
}

bool PlatformTcpSocket::bytes_available() {
    return sock_.bytes_available();
}
