#include "ao/game/effects/SlideEffect.h"

#include "render/Layer.h"

void SlideEffect::configure(Direction direction, int duration_ms) {
    // Departing scene exits in the given direction; arriving enters from the opposite.
    float exit_x = (direction == Direction::LEFT) ? -2.0f : 2.0f;
    float enter_x = -exit_x;

    out_anim_.clear_keyframes();
    out_anim_.set_easing(Easing::CUBIC_IN_OUT);
    out_anim_.set_looping(false);
    out_anim_.add_keyframe({0, {0, 0}, {1, 1}, 0});
    out_anim_.add_keyframe({duration_ms, {exit_x, 0}, {1, 1}, 0});

    in_anim_.clear_keyframes();
    in_anim_.set_easing(Easing::CUBIC_IN_OUT);
    in_anim_.set_looping(false);
    in_anim_.add_keyframe({0, {enter_x, 0}, {1, 1}, 0});
    in_anim_.add_keyframe({duration_ms, {0, 0}, {1, 1}, 0});

    enter_x_ = enter_x;
}

void SlideEffect::trigger() {
    phase_ = Phase::PRE_DELAY;
    delay_remaining_ms_ = BOOKEND_DELAY_MS;
}

void SlideEffect::stop() {
    phase_ = Phase::INACTIVE;
    out_anim_.stop();
    out_anim_.reset();
    in_anim_.stop();
    in_anim_.reset();
}

void SlideEffect::tick(int delta_ms) {
    switch (phase_) {
    case Phase::INACTIVE:
        return;

    case Phase::PRE_DELAY:
        delay_remaining_ms_ -= delta_ms;
        if (delay_remaining_ms_ <= 0) {
            phase_ = Phase::SLIDING;
            out_anim_.play();
            in_anim_.play();
            // Apply leftover time from this frame to the animators
            int leftover = -delay_remaining_ms_;
            out_anim_.tick(leftover);
            in_anim_.tick(leftover);
        }
        break;

    case Phase::SLIDING:
        out_anim_.tick(delta_ms);
        in_anim_.tick(delta_ms);
        if (out_anim_.is_finished()) {
            phase_ = Phase::POST_DELAY;
            delay_remaining_ms_ = BOOKEND_DELAY_MS;
        }
        break;

    case Phase::POST_DELAY:
        delay_remaining_ms_ -= delta_ms;
        if (delay_remaining_ms_ <= 0) {
            phase_ = Phase::INACTIVE;
        }
        break;
    }
}

void SlideEffect::apply(LayerGroup&) {
    // No-op — use apply_out/apply_in on separate layer groups.
}

void SlideEffect::apply_out(LayerGroup& scene) {
    if (phase_ == Phase::SLIDING)
        out_anim_.apply(scene.transform());
    // During pre-delay, departing scene stays at origin (no transform needed)
}

void SlideEffect::apply_in(LayerGroup& scene) {
    if (phase_ == Phase::SLIDING) {
        in_anim_.apply(scene.transform());
    }
    else if (phase_ == Phase::PRE_DELAY) {
        // Arriving scene is off-screen during pre-delay
        scene.transform().translate({enter_x_, 0});
    }
}

bool SlideEffect::needs_departing_scene() const {
    return phase_ == Phase::PRE_DELAY || phase_ == Phase::SLIDING;
}

bool SlideEffect::is_active() const {
    return phase_ != Phase::INACTIVE;
}
