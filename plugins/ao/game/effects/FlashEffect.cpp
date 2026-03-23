#include "ao/game/effects/FlashEffect.h"

#include "render/Layer.h"

#include <algorithm>
#include <vector>

FlashEffect::FlashEffect(int viewport_w, int viewport_h) {
    // Create a 1-frame solid white image asset
    std::vector<uint8_t> pixels(viewport_w * viewport_h * 4, 255);

    DecodedFrame frame;
    frame.width = viewport_w;
    frame.height = viewport_h;
    frame.duration_ms = 0;
    frame.pixels = std::move(pixels);

    overlay_ = std::make_shared<ImageAsset>("_flash_overlay", "raw", std::vector<DecodedFrame>{std::move(frame)});
}

void FlashEffect::trigger() {
    elapsed_ms_ = 0;
    active_ = true;
}

void FlashEffect::tick(int delta_ms) {
    if (!active_)
        return;

    elapsed_ms_ += delta_ms;
    if (elapsed_ms_ >= DURATION_MS)
        active_ = false;
}

void FlashEffect::apply(LayerGroup& scene) {
    float t = std::clamp(static_cast<float>(elapsed_ms_) / DURATION_MS, 0.0f, 1.0f);
    // Quadratic ease-out: fast initial flash, smooth fade
    float opacity = (1.0f - t) * (1.0f - t);

    Layer flash(overlay_, 0, Z_INDEX);
    flash.set_opacity(opacity);
    scene.add_layer(Z_INDEX, std::move(flash));
}

bool FlashEffect::is_active() const {
    return active_;
}
