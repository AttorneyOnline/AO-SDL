#pragma once

#include "Event.h"

#include <string>
#include <vector>

class CharacterListEvent : public Event {
  public:
    explicit CharacterListEvent(std::vector<std::string> characters);

    const std::vector<std::string>& get_characters() const;
    std::string to_string() const override;

  private:
    std::vector<std::string> m_characters;
};
