#pragma once

#include "ao/game/effects/ISceneEffect.h"
#include "render/TransformAnimator.h"

class ScreenshakeEffect : public ISceneEffect {
  public:
    void trigger() override;
    void tick(int delta_ms) override;
    void apply(LayerGroup& scene) override;
    bool is_active() const override;

  private:
    TransformAnimator anim_;
};
