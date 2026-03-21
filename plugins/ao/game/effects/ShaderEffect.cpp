#include "ao/game/effects/ShaderEffect.h"

#include "asset/MediaManager.h"
#include "render/Layer.h"

ShaderEffect::ShaderEffect(std::string shader_path, float duration_s, int layer_id)
    : shader_path_(std::move(shader_path)), duration_s_(duration_s), layer_id_(layer_id) {
}

void ShaderEffect::trigger() {
    if (!shader_) {
        shader_ = MediaManager::instance().assets().shader(shader_path_);
        if (shader_)
            shader_->set_uniform_provider(time_provider_);
    }

    active_ = true;
    elapsed_s_ = 0;
}

void ShaderEffect::tick(int delta_ms) {
    if (!active_) return;

    elapsed_s_ += delta_ms / 1000.0f;
    time_provider_->time = elapsed_s_;

    if (duration_s_ > 0 && elapsed_s_ >= duration_s_)
        active_ = false;
}

void ShaderEffect::apply(LayerGroup& scene) {
    if (!shader_) return;

    if (layer_id_ >= 0) {
        if (auto* layer = scene.get_layer(layer_id_))
            layer->set_shader(shader_);
    } else {
        scene.set_shader(shader_);
    }
}

bool ShaderEffect::is_active() const {
    return active_ && shader_ != nullptr;
}
