#pragma once

#include "AOBlipPlayer.h"
#include "AOEmotePlayer.h"
#include "asset/ImageAsset.h"

#include <memory>
#include <string>

/// Per-IC-message state. Present while a message is active, cleared when
/// the message queue moves on. Each IC state owns its own emote and blip
/// players so that during slide transitions, the departing character's
/// animation can keep running alongside the arriving character's.
struct ActiveICState {
    bool flip = false;
    bool show_desk = true;
    bool preanim_blocking = false;
    bool slide_pending = false; // textbox deferred until slide finishes
    std::string pending_showname;
    std::string pending_message;
    int pending_text_color = 0;
    bool pending_additive = false;

    AOEmotePlayer emote_player;
    AOBlipPlayer blip_player;
    int prev_chars_visible = 0;
    std::string position;

    // Snapshot of the background/desk for this position, so the departing
    // IC state keeps its own background during slide transitions.
    std::shared_ptr<ImageAsset> bg_asset;
    std::shared_ptr<ImageAsset> desk_asset;
};
