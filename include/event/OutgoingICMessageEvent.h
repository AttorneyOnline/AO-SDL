#pragma once

#include "event/Event.h"

#include <string>

struct ICMessageData {
    int desk_mod = 1;
    std::string pre_emote;
    std::string character;
    std::string emote;
    std::string message;
    std::string side = "def";
    std::string sfx_name;
    int emote_mod = 0;
    int char_id = -1;
    int sfx_delay = 0;
    int objection_mod = 0;
    int evidence_id = 0;
    int flip = 0;
    int realization = 0;
    int text_color = 0;
    std::string showname;
    int other_charid = -1;
    std::string other_name;
    std::string other_emote;
    std::string self_offset = "0&0";
    std::string other_offset = "0&0";
    int other_flip = 0;
    int immediate = 0;
    int looping_sfx = 0;
    int screenshake = 0;
    std::string frame_screenshake;
    std::string frame_realization;
    std::string frame_sfx;
    int additive = 0;
    std::string effects;
    std::string blipname;
    std::string slide;
};

class OutgoingICMessageEvent : public Event {
  public:
    explicit OutgoingICMessageEvent(ICMessageData data);

    std::string to_string() const override;

    const ICMessageData& data() const {
        return data_;
    }

  private:
    ICMessageData data_;
};
