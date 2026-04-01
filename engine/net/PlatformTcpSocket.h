#pragma once

#include "net/ITcpSocket.h"
#include "platform/Socket.h"

#include <cstdint>
#include <string>
#include <vector>

class PlatformTcpSocket : public ITcpSocket {
  public:
    PlatformTcpSocket(const std::string& host, uint16_t port);

    /// Construct from an already-connected platform socket (from server accept).
    explicit PlatformTcpSocket(platform::Socket&& connected_sock);

    /// Enable TLS for this socket. Must be called before connect().
    /// The hostname is used for SNI and certificate verification.
    void enable_ssl(const std::string& hostname);

    int fd() const override;

    void connect() override;
    void set_non_blocking(bool non_blocking) override;
    void send(const uint8_t* data, size_t size) override;
    std::vector<uint8_t> recv() override;
    bool bytes_available() override;

  private:
    static constexpr size_t RECV_BUF_SIZE = 8192;
    std::string host_;
    uint16_t port_;
    std::string ssl_hostname_; // non-empty = upgrade to TLS after connect
    platform::Socket sock_;
};
