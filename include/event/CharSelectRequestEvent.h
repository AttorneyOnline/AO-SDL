/**
 * @file CharSelectRequestEvent.h
 * @brief Event representing a user's character selection request.
 * @ingroup events
 */
#pragma once

#include "Event.h"

/**
 * @brief Requests selection of a character by the local player.
 * @ingroup events
 *
 * Published when the user picks a character from the selection screen.
 * The character is identified by its index in the server's character list.
 */
class CharSelectRequestEvent : public Event {
  public:
    /**
     * @brief Constructs a CharSelectRequestEvent.
     * @param char_id Zero-based index of the selected character in the
     *                server's character list.
     */
    explicit CharSelectRequestEvent(int char_id);

    /**
     * @brief Gets the selected character index.
     * @return The zero-based character ID.
     */
    int get_char_id() const;

  private:
    int char_id; /**< Index of the selected character. */
};
