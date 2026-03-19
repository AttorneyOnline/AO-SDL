/**
 * @file UIEvent.h
 * @brief Event signalling a UI screen transition.
 * @ingroup events
 */
#pragma once

#include "Event.h"

/**
 * @brief Enumeration of UI transition signals.
 * @ingroup events
 */
enum UIEventType {
    CHAR_LOADING_DONE,  /**< Character loading has finished. */
    ENTERED_COURTROOM   /**< The player has entered the courtroom screen. */
};

/**
 * @brief Signals a UI screen transition.
 * @ingroup events
 *
 * Published when the application transitions between major screens
 * (e.g., character loading completes or the courtroom is entered).
 */
class UIEvent : public Event {
  public:
    /**
     * @brief Constructs a UIEvent.
     * @param type The transition type that occurred.
     */
    UIEvent(UIEventType type);

    /**
     * @brief Returns a human-readable representation of the UI event.
     * @return String describing the transition type.
     */
    std::string to_string() const override;

    /**
     * @brief Gets the UI transition type.
     * @return The UIEventType value.
     */
    UIEventType get_type();

  private:
    UIEventType type; /**< The transition type. */
};
