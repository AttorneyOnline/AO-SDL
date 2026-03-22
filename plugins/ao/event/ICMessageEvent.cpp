#include "ao/event/ICMessageEvent.h"

#include <format>

ICMessageEvent::ICMessageEvent(std::string character, std::string emote, std::string pre_emote, std::string message,
                               std::string showname, std::string side, EmoteMod emote_mod, DeskMod desk_mod, bool flip,
                               int char_id, int text_color, int objection_mod, bool screenshake, bool realization,
                               bool additive, std::string frame_screenshake, std::string sfx_name, int sfx_delay,
                               bool sfx_looping, std::string frame_sfx)
    : character(std::move(character)), emote(std::move(emote)), pre_emote(std::move(pre_emote)),
      message(std::move(message)), showname(std::move(showname)), side(std::move(side)), emote_mod(emote_mod),
      desk_mod(desk_mod), flip(flip), char_id(char_id), text_color(text_color), objection_mod(objection_mod),
      screenshake(screenshake), realization(realization), additive(additive),
      frame_screenshake(std::move(frame_screenshake)), sfx_name(std::move(sfx_name)), sfx_delay(sfx_delay),
      sfx_looping(sfx_looping), frame_sfx(std::move(frame_sfx)) {
}

std::string ICMessageEvent::to_string() const {
    return std::format("ICMessageEvent(char={}, emote={}, side={}, mod={})", character, emote, side,
                       static_cast<int>(emote_mod));
}
