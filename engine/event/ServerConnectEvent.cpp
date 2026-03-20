#include "event/ServerConnectEvent.h"

ServerConnectEvent::ServerConnectEvent(std::string host, uint16_t port) : host(std::move(host)), port(port) {
}

const std::string& ServerConnectEvent::get_host() const {
    return host;
}

uint16_t ServerConnectEvent::get_port() const {
    return port;
}
