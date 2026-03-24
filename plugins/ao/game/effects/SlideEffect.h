#pragma once

#include "ao/game/effects/ISceneEffect.h"
#include "render/TransformAnimator.h"

/// Horizontal pan transition between courtroom positions.
///
/// Drives two animations: the departing scene slides out while the
/// arriving scene slides in. Both are synchronized with the same
/// phase timing (pre-delay, slide, post-delay).
class SlideEffect : public ISceneEffect {
  public:
    enum class Direction { LEFT, RIGHT };

    /// Configure a slide transition.
    /// @param direction Which way the departing scene exits.
    /// @param duration_ms Pan duration in milliseconds.
    void configure(Direction direction, int duration_ms);

    void trigger() override;
    void stop() override;
    void tick(int delta_ms) override;
    bool is_active() const override;

    /// Apply is a no-op — use apply_out/apply_in on separate layer groups.
    void apply(LayerGroup& scene) override;

    /// True during PRE_DELAY and SLIDING — the departing scene should be visible.
    bool needs_departing_scene() const;

    /// Apply the departing (slide-out) transform.
    void apply_out(LayerGroup& scene);

    /// Apply the arriving (slide-in) transform.
    void apply_in(LayerGroup& scene);

  private:
    TransformAnimator out_anim_;
    TransformAnimator in_anim_;
    float enter_x_ = 2.0f; // arriving scene's off-screen start position

    enum class Phase { INACTIVE, PRE_DELAY, SLIDING, POST_DELAY };
    Phase phase_ = Phase::INACTIVE;
    int delay_remaining_ms_ = 0;

    static constexpr int BOOKEND_DELAY_MS = 300;
};
