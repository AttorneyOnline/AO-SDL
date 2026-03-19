#pragma once

#include "Event.h"

#include <cstdint>
#include <string>

class ServerConnectEvent : public Event {
  public:
    ServerConnectEvent(std::string host, uint16_t port);
    const std::string& get_host() const;
    uint16_t get_port() const;

  private:
    std::string m_host;
    uint16_t m_port;
};
