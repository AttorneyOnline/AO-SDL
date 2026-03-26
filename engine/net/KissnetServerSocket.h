#pragma once

#include "net/IServerSocket.h"

#include <memory>
#include <string>

class KissnetServerSocket : public IServerSocket {
  public:
    explicit KissnetServerSocket(const std::string& bind_address = "0.0.0.0");
    ~KissnetServerSocket() override;

    void bind_and_listen(uint16_t port, int backlog = 128) override;
    std::unique_ptr<ITcpSocket> accept() override;
    void set_non_blocking(bool non_blocking) override;
    void close() override;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::string bind_address_;
};
