#include "CharacterListEvent.h"

#include <format>

CharacterListEvent::CharacterListEvent(std::vector<std::string> characters)
    : m_characters(std::move(characters)) {
}

const std::vector<std::string>& CharacterListEvent::get_characters() const {
    return m_characters;
}

std::string CharacterListEvent::to_string() const {
    return std::format("CharacterListEvent ({} characters)", m_characters.size());
}
