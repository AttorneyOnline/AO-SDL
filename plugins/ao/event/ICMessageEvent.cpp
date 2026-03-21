#include "ao/event/ICMessageEvent.h"

#include <format>

ICMessageEvent::ICMessageEvent(std::string character, std::string emote, std::string pre_emote, std::string message,
                               std::string showname, std::string side, EmoteMod emote_mod, DeskMod desk_mod, bool flip,
                               int char_id, int text_color, bool screenshake, std::string frame_screenshake)
    : character(std::move(character)), emote(std::move(emote)), pre_emote(std::move(pre_emote)),
      message(std::move(message)), showname(std::move(showname)), side(std::move(side)), emote_mod(emote_mod),
      desk_mod(desk_mod), flip(flip), char_id(char_id), text_color(text_color), screenshake(screenshake),
      frame_screenshake(std::move(frame_screenshake)) {
}

std::string ICMessageEvent::to_string() const {
    return std::format("ICMessageEvent(char={}, emote={}, side={}, mod={})", character, emote, side,
                       static_cast<int>(emote_mod));
}
