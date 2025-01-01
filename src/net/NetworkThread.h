#ifndef NETWORKTHREAD_H
#define NETWORKTHREAD_H

#include "WebSocket.h"

#include <thread>

class NetworkThread {
  public:
    NetworkThread(WebSocket sock);

  private:
    std::thread net_thread;
    WebSocket sock;
};

#endif