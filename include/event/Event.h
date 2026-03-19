/**
 * @file Event.h
 * @brief Base class for all events in the event system.
 * @defgroup events Event System
 * @{
 */
#pragma once

#include <string>

/**
 * @brief Abstract base class for all events.
 *
 * All concrete event types must inherit from this class in order to be
 * published and consumed through EventChannel and EventManager.
 */
class Event {
  public:
    /** @brief Default constructor. */
    Event() = default;

    /**
     * @brief Returns a human-readable string representation of the event.
     * @return A string describing the event (type and payload summary).
     */
    virtual std::string to_string() const;
};

/** @} */
