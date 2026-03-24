/**
 * @file SessionEndEvent.h
 * @brief Published by the network thread after a server connection ends.
 * @ingroup events
 */
#pragma once

#include "Event.h"

/// Signals that the current server session has ended.
/// Consumed by the main thread to destroy the Session object.
class SessionEndEvent : public Event {
  public:
    std::string to_string() const override {
        return "SessionEndEvent";
    }
};
