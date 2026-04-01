#include "net/PlatformServerSocket.h"

#include "net/PlatformTcpSocket.h"

#include <stdexcept>

PlatformServerSocket::PlatformServerSocket(const std::string& bind_address) : bind_address_(bind_address) {
}

PlatformServerSocket::~PlatformServerSocket() = default;

int PlatformServerSocket::fd() const {
    return sock_.fd();
}

void PlatformServerSocket::bind_and_listen(uint16_t port, int backlog) {
    sock_ = platform::tcp_listen(bind_address_, port, backlog);
}

std::unique_ptr<ITcpSocket> PlatformServerSocket::accept() {
    if (!sock_.valid()) {
        throw std::runtime_error("Server socket not bound");
    }

    std::string remote_addr;
    uint16_t remote_port = 0;
    auto client = platform::tcp_accept(sock_, remote_addr, remote_port);
    if (!client.valid()) {
        return nullptr;
    }

    return std::make_unique<PlatformTcpSocket>(std::move(client));
}

void PlatformServerSocket::set_non_blocking(bool non_blocking) {
    if (sock_.valid()) {
        sock_.set_non_blocking(non_blocking);
    }
}

void PlatformServerSocket::close() {
    sock_.close();
}
