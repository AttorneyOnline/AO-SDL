#pragma once

#include "ao/asset/AOCharacterSheet.h"
#include "render/Texture.h"

#include <optional>
#include <string>
#include <vector>

struct EmoteIcon {
    std::string comment;
    std::optional<Texture2D> icon;
};

struct ICMessageState {
    // Character info (populated when entering courtroom)
    std::string character;
    int char_id = -1;
    std::optional<AOCharacterSheet> char_sheet;
    std::vector<EmoteIcon> emote_icons;

    // ICChatWidget
    char message[1024] = "";
    char showname[32] = "";

    // EmoteSelectorWidget
    int selected_emote = 0;

    // InterjectionWidget
    int objection_mod = 0;

    // SideSelectWidget
    int side_index = 0;

    // MessageOptionsWidget
    bool flip = false;
    bool pre_anim = false;
    int text_color = 0;
    bool realization = false;
    bool screenshake = false;
    bool additive = false;
};
