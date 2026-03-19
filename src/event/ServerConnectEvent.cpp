#include "ServerConnectEvent.h"

ServerConnectEvent::ServerConnectEvent(std::string host, uint16_t port)
    : m_host(std::move(host)), m_port(port) {
}

const std::string& ServerConnectEvent::get_host() const {
    return m_host;
}

uint16_t ServerConnectEvent::get_port() const {
    return m_port;
}
