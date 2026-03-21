#pragma once

#include "ao/game/effects/ISceneEffect.h"
#include "asset/ShaderAsset.h"

#include <memory>
#include <string>

/// Generic time-driven shader effect applied to a specific layer.
/// Loads a shader from the asset system, feeds u_time, and applies it
/// for a configurable duration. Suitable for any effect whose only
/// custom input is elapsed time.
class ShaderEffect : public ISceneEffect {
  public:
    /// @param shader_path  Asset path (e.g. "shaders/rainbow").
    /// @param duration_s   How long the effect plays (0 = infinite).
    /// @param layer_id     Which layer to apply to, or -1 for the whole group.
    ShaderEffect(std::string shader_path, float duration_s, int layer_id = 5);

    void trigger() override;
    void tick(int delta_ms) override;
    void apply(LayerGroup& scene) override;
    bool is_active() const override;

  private:
    class TimeProvider : public ShaderUniformProvider {
      public:
        float time = 0;
        std::unordered_map<std::string, UniformValue> get_uniforms() const override {
            return {{"u_time", time}};
        }
    };

    std::string shader_path_;
    float duration_s_;
    int layer_id_;

    std::shared_ptr<ShaderAsset> shader_;
    std::shared_ptr<TimeProvider> time_provider_ = std::make_shared<TimeProvider>();
    bool active_ = false;
    float elapsed_s_ = 0;
};
