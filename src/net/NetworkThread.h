#pragma once

#include "ProtocolHandler.h"

#include <thread>

class NetworkThread {
  public:
    explicit NetworkThread(ProtocolHandler& handler);

  private:
    void net_loop();

    ProtocolHandler& handler;
    std::thread net_thread;
};
