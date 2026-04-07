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

    /// Construct from an accepted socket with known remote address.
    PlatformTcpSocket(platform::Socket&& connected_sock, std::string remote_addr);

    /// Enable TLS for this socket. Must be called before connect().
    /// The hostname is used for SNI and certificate verification.
    void enable_ssl(const std::string& hostname);

    /// Set the TCP connect timeout in milliseconds.
    /// Must be called before connect(). Default: 10000ms (10 seconds).
    void set_connect_timeout(int timeout_ms);

    int fd() const override;
    std::string remote_addr() const override;

    void connect() override;
    void set_non_blocking(bool non_blocking) override;
    void send(const uint8_t* data, size_t size) override;
    std::vector<uint8_t> recv() override;
    bool bytes_available() override;

  private:
    static constexpr size_t RECV_BUF_SIZE = 8192;
    std::string host_;
    uint16_t port_;
    std::string ssl_hostname_;       // non-empty = upgrade to TLS after connect
    int connect_timeout_ms_ = 10000; // 10 second default
    platform::Socket sock_;
    std::string remote_addr_;
};
