#include "ao/game/effects/ScreenshakeEffect.h"

#include "render/Layer.h"

#include <random>

void ScreenshakeEffect::trigger() {
    constexpr int DURATION_MS = 300;
    constexpr int JOLT_MS = 20;
    constexpr float MAX_DEV = 7.0f / 192.0f * 2.0f; // NDC range [-1,1] = 2 units

    anim_.clear_keyframes();
    anim_.set_easing(Easing::LINEAR);
    anim_.set_looping(false);

    thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(-MAX_DEV, MAX_DEV);

    int t = 0;
    while (t < DURATION_MS) {
        anim_.add_keyframe({t, {dist(rng), dist(rng)}, {1, 1}, 0});
        t += JOLT_MS;
    }

    anim_.add_keyframe({DURATION_MS, {0, 0}, {1, 1}, 0});
    anim_.play();
}

void ScreenshakeEffect::tick(int delta_ms) {
    anim_.tick(delta_ms);
}

void ScreenshakeEffect::apply(LayerGroup& scene) {
    anim_.apply(scene.transform());
}

bool ScreenshakeEffect::is_active() const {
    return anim_.is_playing();
}
