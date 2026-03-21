#pragma once

#include "asset/ImageAsset.h"
#include "render/Transform.h"

#include <cstdint>
#include <map>
#include <memory>

class Layer {
  public:
    Layer(std::shared_ptr<ImageAsset> asset, int frame_index, uint16_t z_index);

    const std::shared_ptr<ImageAsset>& get_asset() const { return asset; }
    int get_frame_index() const { return frame_index; }
    uint16_t get_z_index() const { return z_index; }

    Transform& transform() { return transform_; }
    const Transform& transform() const { return transform_; }

  private:
    std::shared_ptr<ImageAsset> asset;
    int frame_index;
    uint16_t z_index;
    Transform transform_;
};

class LayerGroup {
  public:
    LayerGroup();

    void add_layer(int id, Layer layer);
    const std::map<int, Layer>& get_layers() const { return layers; }

    Transform& transform() { return transform_; }
    const Transform& transform() const { return transform_; }

  private:
    std::map<int, Layer> layers;
    Transform transform_;
};
