#include "net/KissnetServerSocket.h"

#include "net/KissnetTcpSocket.h"

#include <kissnet.hpp>

#include <optional>
#include <stdexcept>

struct KissnetServerSocket::Impl {
    std::optional<kissnet::tcp_socket> sock;
};

KissnetServerSocket::KissnetServerSocket(const std::string& bind_address)
    : impl_(std::make_unique<Impl>()), bind_address_(bind_address) {
}

KissnetServerSocket::~KissnetServerSocket() = default;

void KissnetServerSocket::bind_and_listen(uint16_t port, int /*backlog*/) {
    impl_->sock.emplace(kissnet::endpoint(bind_address_, port));
    impl_->sock->bind();
    impl_->sock->listen();
}

std::unique_ptr<ITcpSocket> KissnetServerSocket::accept() {
    if (!impl_->sock)
        throw std::runtime_error("Server socket not bound");

    auto client_sock = impl_->sock->accept();
    if (!client_sock.is_valid())
        return nullptr;

    return std::make_unique<KissnetTcpSocket>(std::move(client_sock));
}

void KissnetServerSocket::set_non_blocking(bool non_blocking) {
    if (impl_->sock)
        impl_->sock->set_non_blocking(non_blocking);
}

void KissnetServerSocket::close() {
    if (impl_->sock) {
        impl_->sock->close();
        impl_->sock.reset();
    }
}
