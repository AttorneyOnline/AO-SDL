/**
 * @file AnimationPlayer.h
 * @brief Tick-driven animation frame sequencer for ImageAsset.
 */
#pragma once

#include "asset/ImageAsset.h"

#include <memory>

/**
 * @brief Plays back frames of an ImageAsset based on elapsed time.
 *
 * Each call to tick() advances the animation by the given delta. The player
 * tracks which frame should be displayed and whether the animation has
 * completed (for one-shot mode). Looping animations never complete.
 *
 * This is a pure logic object — it does not touch the GPU. It returns
 * a pointer to the current frame's pixel data for the renderer to use.
 */
class AnimationPlayer {
  public:
    AnimationPlayer() = default;

    /**
     * @brief Load an image asset for playback.
     * @param asset  The image asset to play. Null clears the player.
     * @param loop   If true, the animation loops. If false, it plays once
     *               and finished() returns true when done.
     */
    void load(std::shared_ptr<ImageAsset> asset, bool loop);

    /** @brief Clear the player — no animation, no frames. */
    void clear();

    /**
     * @brief Advance the animation by the given time delta.
     * @param delta_ms Milliseconds elapsed since the last tick.
     */
    void tick(int delta_ms);

    /** @brief Whether the animation has finished (one-shot only). Always false for looping. */
    bool finished() const {
        return done;
    }

    /** @brief Whether the player has a loaded asset with at least one frame. */
    bool has_frame() const;

    /** @brief Get the current frame index. */
    int current_frame_index() const {
        return frame_index;
    }

    /** @brief Get the current frame's pixel data. Null if nothing loaded. */
    const ImageFrame* current_frame() const;

    /** @brief Get the underlying asset (for lifetime management and GPU upload). */
    const std::shared_ptr<ImageAsset>& asset() const {
        return current_asset;
    }

  private:
    std::shared_ptr<ImageAsset> current_asset;
    int frame_index = 0;
    int elapsed_ms = 0;
    bool looping = false;
    bool done = false;
};
