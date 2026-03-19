#pragma once

#include "ao/event/ICMessageEvent.h"  // EmoteMod
#include "asset/AssetLibrary.h"
#include "asset/ImageAsset.h"
#include "render/AnimationPlayer.h"
#include "render/Image.h"

#include <memory>
#include <optional>
#include <string>

/// AO2 character emote state machine.
///
/// Manages the preanim → talking → idle animation sequence using AO2 file
/// conventions: pre-animation uses the pre_emote name directly, idle uses
/// (a){emote}, talking uses (b){emote}.
///
/// The caller drives time via tick() and reads the current frame for rendering.
class AOEmotePlayer {
  public:
    enum class State { NONE, PREANIM, TALKING, IDLE };

    /// Start a new emote sequence. Loads all animation variants and enters
    /// the appropriate initial state based on emote_mod.
    ///
    /// @param assets      Asset library for loading character sprites.
    /// @param character   Character folder name (e.g. "Phoenix").
    /// @param emote       Base emote name (e.g. "normal").
    /// @param pre_emote   Pre-animation name (e.g. "objecting"), or "-"/empty to skip.
    /// @param emote_mod   Animation mode controlling the state sequence.
    void start(AssetLibrary& assets, const std::string& character,
               const std::string& emote, const std::string& pre_emote,
               EmoteMod emote_mod);

    /// Advance the animation by delta_ms. Handles state transitions
    /// (preanim finished → talking, etc.)
    void tick(int delta_ms);

    /// Current animation state.
    State state() const { return current_state; }

    /// Current frame to render, or nullptr if nothing loaded.
    const ImageFrame* current_frame() const;

    /// Current frame index within the active animation.
    int current_frame_index() const;

    /// The ImageAsset for the currently active animation (for GPU upload).
    const std::shared_ptr<ImageAsset>& asset() const;

    /// Whether a character is currently loaded and displayable.
    bool has_frame() const { return current_frame() != nullptr; }

  private:
    State current_state = State::NONE;

    AnimationPlayer preanim;
    AnimationPlayer talk;
    AnimationPlayer idle;
};
