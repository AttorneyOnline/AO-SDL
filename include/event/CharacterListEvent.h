/**
 * @file CharacterListEvent.h
 * @brief Event carrying the list of character folder names from the server.
 * @ingroup events
 */
#pragma once

#include "Event.h"

#include <string>
#include <vector>

/**
 * @brief Carries the list of character folder names received from the server.
 * @ingroup events
 *
 * Published after the server sends the full character roster. Each entry
 * is the folder name that identifies a playable character.
 */
class CharacterListEvent : public Event {
  public:
    /**
     * @brief Constructs a CharacterListEvent.
     * @param characters Vector of character folder name strings.
     */
    explicit CharacterListEvent(std::vector<std::string> characters);

    /**
     * @brief Gets the character folder name list.
     * @return Const reference to the vector of character names.
     */
    const std::vector<std::string>& get_characters() const;

    /**
     * @brief Returns a human-readable representation of the event.
     * @return String summary including the number of characters.
     */
    std::string to_string() const override;

  private:
    std::vector<std::string> characters; /**< Character folder names. */
};
