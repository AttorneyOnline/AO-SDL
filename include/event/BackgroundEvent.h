/**
 * @file BackgroundEvent.h
 * @brief Event carrying a courtroom background change.
 * @ingroup events
 */
#pragma once

#include "Event.h"

#include <string>

/**
 * @brief Signals that the courtroom background should change.
 * @ingroup events
 *
 * Published by the protocol plugin when the server sends a background
 * change command (e.g. the AO2 BN packet). The courtroom presenter
 * consumes this to load and display the new background.
 */
class BackgroundEvent : public Event {
  public:
    /**
     * @param background The background folder name (e.g. "default", "gs4").
     * @param position   The courtroom position (e.g. "def", "pro", "wit").
     *                   Empty string means keep the current position.
     */
    BackgroundEvent(std::string background, std::string position);

    std::string to_string() const override;

    /** @brief Background folder name. */
    const std::string& get_background() const { return background; }

    /** @brief Courtroom position (may be empty). */
    const std::string& get_position() const { return position; }

  private:
    std::string background;
    std::string position;
};
