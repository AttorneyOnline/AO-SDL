#pragma once

#include "ao/asset/AOAssetLibrary.h"
#include "event/ICMessageEvent.h" // EmoteMod
#include "asset/ImageAsset.h"
#include "render/AnimationPlayer.h"

#include <memory>
#include <string>

/// AO2 character emote state machine.
///
/// Manages the preanim → talking → idle animation sequence.
/// Delegates all path resolution to AOAssetLibrary.
class AOEmotePlayer {
  public:
    enum class State { NONE, PREANIM, TALKING, IDLE };

    void start(AOAssetLibrary& ao_assets, const std::string& character, const std::string& emote,
               const std::string& pre_emote, EmoteMod emote_mod);

    void tick(int delta_ms);

    /// Retry loading assets that were nullptr (e.g. HTTP download completed).
    /// Returns true if any asset was newly loaded.
    bool retry_load(AOAssetLibrary& ao_assets);

    /// Transition from TALKING to IDLE (called when text finishes scrolling).
    void transition_to_idle();

    /// Stop all animation and reset to NONE.
    void stop() {
        current_state = State::NONE;
    }

    State state() const {
        return current_state;
    }
    const ImageFrame* current_frame() const;
    int current_frame_index() const;
    const std::shared_ptr<ImageAsset>& asset() const;
    bool has_frame() const {
        return current_frame() != nullptr;
    }

  private:
    State current_state = State::NONE;
    bool needs_retry_ = false;
    int retry_count_ = 0;
    bool fallback_prefetched_ = false;

    // Stored for retry
    std::string character_;
    std::string emote_;
    std::string pre_emote_;
    EmoteMod emote_mod_ = EmoteMod::IDLE;

    AnimationPlayer preanim;
    AnimationPlayer talk;
    AnimationPlayer idle;
};
