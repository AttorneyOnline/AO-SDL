#pragma once

#include "net/ITcpSocket.h"

#include <kissnet.hpp>

#include <cstdint>
#include <string>
#include <vector>

class KissnetTcpSocket : public ITcpSocket {
  public:
    KissnetTcpSocket(const std::string& host, uint16_t port);

    void connect() override;
    void set_non_blocking(bool non_blocking) override;
    void send(const uint8_t* data, size_t size) override;
    std::vector<uint8_t> recv() override;
    bool bytes_available() override;

  private:
    static constexpr size_t RECV_BUF_SIZE = 8192;
    kissnet::tcp_socket sock;
};
