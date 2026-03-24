/**
 * @file DisconnectRequestEvent.h
 * @brief Event published by the UI to request the network thread close the active connection.
 * @ingroup events
 */
#pragma once

#include "Event.h"

/// Published by the UI layer when the user clicks Disconnect.
/// The network thread consumes this to break out of its read loop
/// and tear down the socket.
class DisconnectRequestEvent : public Event {
  public:
    std::string to_string() const override {
        return "DisconnectRequestEvent";
    }
};
