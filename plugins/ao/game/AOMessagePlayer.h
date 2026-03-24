#pragma once

#include "ActiveICState.h"
#include "ICMessageQueue.h"

#include <string>
#include <vector>

class AOAssetLibrary;
class AOTextBox;

/// Initializes an ActiveICState from an incoming ICMessage.
///
/// Handles character sheet lookup, showname resolution, emote start,
/// preanim blocking, SFX playback, blip setup, and IC log publishing.
/// Returns the initialized state plus a list of effect names to trigger.
class AOMessagePlayer {
  public:
    struct Result {
        ActiveICState ic;
        std::vector<std::string> effects; // names matching presenter's NamedEffect keys
        std::string resolved_showname;    // showname after char.ini fallback
    };

    /// Initialize a new IC state from a message.
    Result play(const ICMessage& msg, AOAssetLibrary& ao_assets, AOTextBox& textbox, bool active);
};
