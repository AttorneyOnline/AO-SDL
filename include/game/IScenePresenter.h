/**
 * @file IScenePresenter.h
 * @brief Interface for game logic producers that generate render state each tick.
 */
#pragma once

#include "render/RenderState.h"

#include <atomic>
#include <cstdint>

/**
 * @brief Abstract interface for scene presenters.
 *
 * A scene presenter encapsulates the game logic for a particular screen or
 * mode. Each call to tick() advances the simulation and produces a RenderState
 * snapshot that the renderer can display.
 *
 * Implementations are driven by GameThread at approximately 60 Hz.
 */
class IScenePresenter {
  public:
    virtual ~IScenePresenter() = default;

    /// Called once on the game thread before the first tick.
    virtual void init() {
    }

    /**
     * @brief Advance the scene and produce a render snapshot.
     * @param delta_ms Milliseconds elapsed since the last tick.
     * @return A RenderState describing what should be drawn this frame.
     */
    virtual RenderState tick(uint64_t delta_ms) = 0;

    /// Signal whether the courtroom view is actively displayed.
    /// Implementations may suppress audio until this is true.
    virtual void set_courtroom_active(bool) {
    }

    /// Set the local player's char_id and slide preference.
    /// Used to force slide on our own echoed messages when the server strips the flag.
    virtual void set_local_player(int /*char_id*/, bool /*slide_enabled*/) {
    }

    struct ProfileEntry {
        const char* name;
        const std::atomic<int>* us;
    };

    /// Override to expose per-section tick profiling. Returns empty span by default.
    virtual std::vector<ProfileEntry> tick_profile() const {
        return {};
    }
};
