#include "ao/event/ICMessageEvent.h"

#include <format>

ICMessageEvent::ICMessageEvent(std::string character, std::string emote, std::string pre_emote,
                               std::string side, EmoteMod emote_mod, DeskMod desk_mod,
                               bool flip, int char_id)
    : character(std::move(character)), emote(std::move(emote)), pre_emote(std::move(pre_emote)),
      side(std::move(side)), emote_mod(emote_mod), desk_mod(desk_mod),
      flip(flip), char_id(char_id) {}

std::string ICMessageEvent::to_string() const {
    return std::format("ICMessageEvent(char={}, emote={}, side={}, mod={})",
                       character, emote, side, static_cast<int>(emote_mod));
}
