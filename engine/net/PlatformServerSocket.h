#pragma once

#include "net/IServerSocket.h"
#include "platform/Socket.h"

#include <memory>
#include <string>

class PlatformServerSocket : public IServerSocket {
  public:
    explicit PlatformServerSocket(const std::string& bind_address = "0.0.0.0");
    ~PlatformServerSocket() override;

    int fd() const override { return sock_.fd(); }

    void bind_and_listen(uint16_t port, int backlog = 128) override;
    std::unique_ptr<ITcpSocket> accept() override;
    void set_non_blocking(bool non_blocking) override;
    void close() override;

  private:
    std::string bind_address_;
    platform::Socket sock_;
};
