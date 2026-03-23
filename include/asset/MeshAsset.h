/**
 * @file MeshAsset.h
 * @brief Backend-agnostic mesh asset holding vertex and index data.
 */
#pragma once

#include "Asset.h"

#include <cstdint>
#include <vector>

/// Vertex layout matching the existing GL/Metal quad vertex format.
struct MeshVertex {
    float position[2];
    float texcoord[2];
    float color[3] = {1.0f, 1.0f, 1.0f}; ///< Per-vertex color (white default = no tint).
};

/**
 * @brief A mesh asset containing vertex and index data for GPU rendering.
 *
 * Analogous to ShaderAsset for shaders or ImageAsset for textures.
 * Renderer backends cache GPU-side buffers keyed on this asset's pointer
 * and generation counter, reuploading only when data changes.
 *
 * The default rendering pipeline uses a fullscreen quad. When a MeshAsset
 * is attached to a Layer, the renderer draws with this mesh instead.
 */
class MeshAsset : public Asset {
  public:
    MeshAsset(const std::string& path, std::vector<MeshVertex> vertices, std::vector<uint32_t> indices)
        : Asset(path, "mesh"), vertices_(std::move(vertices)), indices_(std::move(indices)) {
    }

    const std::vector<MeshVertex>& vertices() const {
        return vertices_;
    }
    const std::vector<uint32_t>& indices() const {
        return indices_;
    }
    size_t index_count() const {
        return indices_.size();
    }

    /// Update vertex and index data (bumps generation for renderer cache invalidation).
    void update(std::vector<MeshVertex> vertices, std::vector<uint32_t> indices) {
        vertices_ = std::move(vertices);
        indices_ = std::move(indices);
        generation_++;
    }

    uint64_t generation() const {
        return generation_;
    }

    size_t memory_size() const override {
        return vertices_.size() * sizeof(MeshVertex) + indices_.size() * sizeof(uint32_t);
    }

  private:
    std::vector<MeshVertex> vertices_;
    std::vector<uint32_t> indices_;
    uint64_t generation_ = 0;
};
