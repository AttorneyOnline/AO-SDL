#pragma once

#include "ao/game/effects/ISceneEffect.h"
#include "asset/ImageAsset.h"

#include <memory>

/// Full-screen white flash that fades out over ~250ms.
/// Matches the AO2 "realization" visual effect.
class FlashEffect : public ISceneEffect {
  public:
    FlashEffect(int viewport_w, int viewport_h);

    void trigger() override;
    void tick(int delta_ms) override;
    void apply(LayerGroup& scene) override;
    bool is_active() const override;

  private:
    std::shared_ptr<ImageAsset> overlay_;
    int elapsed_ms_ = 0;
    bool active_ = false;

    static constexpr int DURATION_MS = 250;
    static constexpr uint16_t Z_INDEX = 100; // above everything
};
