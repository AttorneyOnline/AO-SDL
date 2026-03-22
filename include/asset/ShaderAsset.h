#pragma once

#include "Asset.h"
#include "render/Math.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

/// Uniform value types that any backend can consume.
using UniformValue = std::variant<int, float, Vec2, Vec3, Mat4>;

/// Interface for feeding custom per-frame uniform data to a shader.
/// Subclass this for effect-specific parameters (tint color, time, etc.).
class ShaderUniformProvider {
  public:
    virtual ~ShaderUniformProvider() = default;
    virtual std::unordered_map<std::string, UniformValue> get_uniforms() const = 0;
};

/// Backend-agnostic shader asset holding vertex + fragment source text.
///
/// Loaded from the virtual filesystem via AssetLibrary::shader().
/// The renderer backend compiles it into a GPU program on first use.
class ShaderAsset : public Asset {
  public:
    ShaderAsset(std::string path, std::string format, std::string vertex_source, std::string fragment_source)
        : Asset(std::move(path), std::move(format)), vertex_source_(std::move(vertex_source)),
          fragment_source_(std::move(fragment_source)) {
    }

    const std::string& vertex_source() const {
        return vertex_source_;
    }
    const std::string& fragment_source() const {
        return fragment_source_;
    }

    void set_uniform_provider(std::shared_ptr<ShaderUniformProvider> p) {
        uniform_provider_ = std::move(p);
    }
    const std::shared_ptr<ShaderUniformProvider>& uniform_provider() const {
        return uniform_provider_;
    }

    size_t memory_size() const override {
        return vertex_source_.size() + fragment_source_.size();
    }

    bool is_default() const {
        return vertex_source_.empty() && fragment_source_.empty();
    }

  private:
    std::string vertex_source_;
    std::string fragment_source_;
    std::shared_ptr<ShaderUniformProvider> uniform_provider_;
};
