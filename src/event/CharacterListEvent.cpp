#include "CharacterListEvent.h"

#include <format>

CharacterListEvent::CharacterListEvent(std::vector<std::string> characters)
    : characters(std::move(characters)) {
}

const std::vector<std::string>& CharacterListEvent::get_characters() const {
    return characters;
}

std::string CharacterListEvent::to_string() const {
    return std::format("CharacterListEvent ({} characters)", characters.size());
}
