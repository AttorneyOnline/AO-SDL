/**
 * @file ServerListEvent.h
 * @brief Event carrying the parsed server list.
 * @ingroup events
 */
#pragma once

#include "Event.h"
#include "game/ServerList.h"

/**
 * @brief Carries a fully parsed server list received from the master server.
 * @ingroup events
 */
class ServerListEvent : public Event {
  public:
    /**
     * @brief Constructs a ServerListEvent.
     * @param server_list The parsed list of available servers.
     */
    ServerListEvent(ServerList server_list);

    /**
     * @brief Returns a human-readable representation of the event.
     * @return String summary of the server list.
     */
    std::string to_string() const override;

    /**
     * @brief Gets the server list payload.
     * @return The ServerList contained in this event.
     */
    ServerList get_server_list();

  private:
    ServerList server_list; /**< The parsed server list. */
};
