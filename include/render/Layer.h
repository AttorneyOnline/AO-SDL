#pragma once

#include "asset/ImageAsset.h"
#include "asset/MeshAsset.h"
#include "render/Transform.h"

#include <cstdint>
#include <map>
#include <memory>

class ShaderAsset;

class Layer {
  public:
    Layer(std::shared_ptr<ImageAsset> asset, int frame_index, uint16_t z_index);

    const std::shared_ptr<ImageAsset>& get_asset() const {
        return asset;
    }
    int get_frame_index() const {
        return frame_index;
    }
    uint16_t get_z_index() const {
        return z_index;
    }

    void set_opacity(float o) {
        opacity = o;
    }
    float get_opacity() const {
        return opacity;
    }

    void set_shader(std::shared_ptr<ShaderAsset> s) {
        shader_ = std::move(s);
    }
    const std::shared_ptr<ShaderAsset>& get_shader() const {
        return shader_;
    }

    void set_mesh(std::shared_ptr<MeshAsset> m) {
        mesh_ = std::move(m);
    }
    const std::shared_ptr<MeshAsset>& get_mesh() const {
        return mesh_;
    }

    Transform& transform() {
        return transform_;
    }
    const Transform& transform() const {
        return transform_;
    }

  private:
    std::shared_ptr<ImageAsset> asset;
    int frame_index;
    uint16_t z_index;
    float opacity = 1.0f;
    std::shared_ptr<ShaderAsset> shader_;
    std::shared_ptr<MeshAsset> mesh_;
    Transform transform_;
};

class LayerGroup {
  public:
    LayerGroup();

    void add_layer(int id, Layer layer);
    Layer* get_layer(int id);
    const std::map<int, Layer>& get_layers() const {
        return layers;
    }

    void set_shader(std::shared_ptr<ShaderAsset> s) {
        shader_ = std::move(s);
    }
    const std::shared_ptr<ShaderAsset>& get_shader() const {
        return shader_;
    }

    Transform& transform() {
        return transform_;
    }
    const Transform& transform() const {
        return transform_;
    }

  private:
    std::map<int, Layer> layers;
    std::shared_ptr<ShaderAsset> shader_;
    Transform transform_;
};
