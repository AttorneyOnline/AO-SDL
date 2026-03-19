/**
 * @file ServerConnectEvent.h
 * @brief Event requesting a connection to a game server.
 * @ingroup events
 */
#pragma once

#include "Event.h"

#include <cstdint>
#include <string>

/**
 * @brief Requests that the client connect to a specific game server.
 * @ingroup events
 */
class ServerConnectEvent : public Event {
  public:
    /**
     * @brief Constructs a ServerConnectEvent.
     * @param host Hostname or IP address of the target server.
     * @param port TCP port number of the target server.
     */
    ServerConnectEvent(std::string host, uint16_t port);

    /**
     * @brief Gets the server hostname or IP address.
     * @return Const reference to the host string.
     */
    const std::string& get_host() const;

    /**
     * @brief Gets the server port number.
     * @return The TCP port.
     */
    uint16_t get_port() const;

  private:
    std::string host; /**< Hostname or IP of the server. */
    uint16_t port;    /**< TCP port of the server. */
};
