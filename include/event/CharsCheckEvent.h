/**
 * @file CharsCheckEvent.h
 * @brief Event carrying per-character taken/free status.
 * @ingroup events
 */
#pragma once

#include "Event.h"

#include <vector>

/**
 * @brief Reports the taken/free status of each character slot.
 * @ingroup events
 *
 * The boolean vector is indexed in the same order as the character list
 * provided by CharacterListEvent. A @c true entry means the character
 * is already taken by another player.
 */
class CharsCheckEvent : public Event {
  public:
    /**
     * @brief Constructs a CharsCheckEvent.
     * @param taken Vector of booleans; @c true if the character at that
     *              index is taken, @c false if free.
     */
    explicit CharsCheckEvent(std::vector<bool> taken);

    /**
     * @brief Gets the taken/free status vector.
     * @return Const reference to the vector of taken flags.
     */
    const std::vector<bool>& get_taken() const;

  private:
    std::vector<bool> taken; /**< Per-character taken flags. */
};
