#pragma once

#include "WebSocket.h"

#include <thread>

class NetworkThread {
  public:
    NetworkThread(WebSocket& sock);

  private:
    void net_loop();

    std::thread net_thread;
    WebSocket& sock;
};
