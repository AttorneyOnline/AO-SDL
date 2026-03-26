#include "net/KissnetTcpSocket.h"

#include "net/ITcpSocket.h"

#include <algorithm>
#include <iterator>

KissnetTcpSocket::KissnetTcpSocket(const std::string& host, uint16_t port) : sock(kissnet::endpoint(host, port)) {
}

KissnetTcpSocket::KissnetTcpSocket(kissnet::tcp_socket&& connected_sock) : sock(std::move(connected_sock)) {
}

void KissnetTcpSocket::connect() {
    sock.connect();
}

void KissnetTcpSocket::set_non_blocking(bool non_blocking) {
    sock.set_non_blocking(non_blocking);
}

void KissnetTcpSocket::send(const uint8_t* data, size_t size) {
    sock.send(reinterpret_cast<const std::byte*>(data), size);
}

std::vector<uint8_t> KissnetTcpSocket::recv() {
    kissnet::buffer<RECV_BUF_SIZE> buf;
    const auto [recv_size, status] = sock.recv(buf);

    if (status.value == kissnet::socket_status::errored) {
        throw WebSocketException("TCP socket read error");
    }
    if (status.value == kissnet::socket_status::cleanly_disconnected) {
        throw WebSocketException("TCP connection closed by peer");
    }

    std::vector<uint8_t> result;
    if (recv_size > 0) {
        result.reserve(recv_size);
        std::transform(buf.begin(), buf.begin() + recv_size, std::back_inserter(result),
                       [](std::byte b) { return static_cast<uint8_t>(b); });
    }
    return result;
}

bool KissnetTcpSocket::bytes_available() {
    return sock.bytes_available();
}
