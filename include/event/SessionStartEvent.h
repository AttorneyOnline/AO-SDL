/**
 * @file SessionStartEvent.h
 * @brief Published by the network thread when a server connection is established.
 * @ingroup events
 */
#pragma once

#include "Event.h"

/// Signals that a new server session has begun.
/// Consumed by the main thread to create a Session object.
class SessionStartEvent : public Event {
  public:
    std::string to_string() const override {
        return "SessionStartEvent";
    }
};
