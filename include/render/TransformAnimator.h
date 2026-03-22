#pragma once

#include "render/Math.h"

#include <vector>

enum class Easing {
    LINEAR,
    QUAD_IN,
    QUAD_OUT,
    QUAD_IN_OUT,
};

struct TransformKeyframe {
    int time_ms = 0;
    Vec2 translation = {0, 0};
    Vec2 scale = {1, 1};
    float rotation = 0;
};

class Transform;

/// Drives a Transform through a sequence of keyframes over time.
///
/// Usage:
///   TransformAnimator anim;
///   anim.add_keyframe({0,    {0,0}, {1,1}, 0});
///   anim.add_keyframe({500,  {1,0}, {2,2}, 45});
///   anim.set_easing(Easing::QUAD_IN_OUT);
///   anim.play();
///
///   // Each frame:
///   anim.tick(delta_ms);
///   anim.apply(my_transform);
class TransformAnimator {
  public:
    void add_keyframe(const TransformKeyframe& kf);
    void clear_keyframes();

    void set_easing(Easing easing) {
        easing_ = easing;
    }
    void set_looping(bool loop) {
        looping_ = loop;
    }

    void play();
    void stop();
    void reset();

    bool is_playing() const {
        return playing_;
    }
    bool is_finished() const {
        return finished_;
    }

    /// Advance the animation by delta_ms. Returns true if the transform changed.
    bool tick(int delta_ms);

    /// Apply the current interpolated state to a Transform.
    void apply(Transform& target) const;

    /// Get the current interpolated values directly.
    Vec2 current_translation() const {
        return current_.translation;
    }
    Vec2 current_scale() const {
        return current_.scale;
    }
    float current_rotation() const {
        return current_.rotation;
    }

  private:
    float ease(float t) const;

    std::vector<TransformKeyframe> keyframes_;
    Easing easing_ = Easing::LINEAR;
    bool looping_ = false;
    bool playing_ = false;
    bool finished_ = false;
    int elapsed_ms_ = 0;

    TransformKeyframe current_ = {};
};
