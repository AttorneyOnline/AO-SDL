#include "render/TransformAnimator.h"

#include "render/Transform.h"

#include <algorithm>

void TransformAnimator::add_keyframe(const TransformKeyframe& kf) {
    keyframes_.push_back(kf);
    std::sort(keyframes_.begin(), keyframes_.end(), [](const auto& a, const auto& b) { return a.time_ms < b.time_ms; });
}

void TransformAnimator::clear_keyframes() {
    keyframes_.clear();
    reset();
}

void TransformAnimator::play() {
    playing_ = true;
    finished_ = false;
}

void TransformAnimator::stop() {
    playing_ = false;
}

void TransformAnimator::reset() {
    elapsed_ms_ = 0;
    playing_ = false;
    finished_ = false;
    if (!keyframes_.empty())
        current_ = keyframes_.front();
}

static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

static Vec2 lerp_vec2(Vec2 a, Vec2 b, float t) {
    return {lerp(a.x, b.x, t), lerp(a.y, b.y, t)};
}

float TransformAnimator::ease(float t) const {
    switch (easing_) {
    case Easing::LINEAR:
        return t;
    case Easing::QUAD_IN:
        return t * t;
    case Easing::QUAD_OUT:
        return t * (2.0f - t);
    case Easing::QUAD_IN_OUT:
        if (t < 0.5f)
            return 2.0f * t * t;
        return -1.0f + (4.0f - 2.0f * t) * t;
    case Easing::CUBIC_IN_OUT: {
        float p = 2.0f * t - 2.0f;
        if (t < 0.5f)
            return 4.0f * t * t * t;
        return 0.5f * p * p * p + 1.0f;
    }
    }
    return t;
}

bool TransformAnimator::tick(int delta_ms) {
    if (!playing_ || finished_ || keyframes_.size() < 2)
        return false;

    elapsed_ms_ += delta_ms;

    int total = keyframes_.back().time_ms;
    if (elapsed_ms_ >= total) {
        if (looping_) {
            elapsed_ms_ %= total;
        }
        else {
            elapsed_ms_ = total;
            current_ = keyframes_.back();
            finished_ = true;
            playing_ = false;
            return true;
        }
    }

    // Find the surrounding keyframes
    size_t next = 0;
    for (size_t i = 1; i < keyframes_.size(); i++) {
        if (keyframes_[i].time_ms >= elapsed_ms_) {
            next = i;
            break;
        }
    }
    size_t prev = next - 1;

    const auto& kf0 = keyframes_[prev];
    const auto& kf1 = keyframes_[next];
    int segment_len = kf1.time_ms - kf0.time_ms;
    float t = (segment_len > 0) ? static_cast<float>(elapsed_ms_ - kf0.time_ms) / segment_len : 1.0f;
    t = ease(t);

    current_.translation = lerp_vec2(kf0.translation, kf1.translation, t);
    current_.scale = lerp_vec2(kf0.scale, kf1.scale, t);
    current_.rotation = lerp(kf0.rotation, kf1.rotation, t);

    return true;
}

void TransformAnimator::apply(Transform& target) const {
    target.translate(current_.translation);
    target.scale(current_.scale);
    target.rotate(current_.rotation);
}
