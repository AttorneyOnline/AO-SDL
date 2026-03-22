#pragma once

#include "ao/asset/AOAssetLibrary.h"
#include "asset/SoundAsset.h"

#include <memory>
#include <string>

/// Manages blip sound scheduling during text advancement.
/// Plays a blip every N non-whitespace characters, resetting
/// the cadence after whitespace runs.
class AOBlipPlayer {
  public:
    /// Reset state for a new message and load the character's blip sound.
    void start(AOAssetLibrary& ao_assets, const std::string& character);

    /// Call each tick after text advances. Publishes PlayBlipEvents as needed.
    /// @param prev_chars  Previous chars_visible count before this tick.
    /// @param cur_chars   Current chars_visible count after this tick.
    /// @param message     The message text (for character inspection).
    /// @param talking     Whether the current color has talking=true.
    /// @param active      Whether the courtroom is active (audio should play).
    void tick(AOAssetLibrary& ao_assets, int prev_chars, int cur_chars, const std::string& message, bool talking,
              bool active);

  private:
    static bool is_whitespace(char c);

    std::shared_ptr<SoundAsset> blip_asset_;
    std::string character_; // for retry loading
    int blip_ticker_ = 0;

    static constexpr int BLIP_RATE = 2; // play blip every N eligible characters
};
