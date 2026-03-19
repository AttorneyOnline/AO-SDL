/**
 * @file Layer.h
 * @brief Layer (asset reference + frame index + z-index) and LayerGroup.
 */
#pragma once

#include "asset/ImageAsset.h"

#include <cstdint>
#include <map>
#include <memory>

/**
 * @brief A renderable layer referencing an ImageAsset and a specific frame.
 *
 * The layer does not hold raw pixel data. It holds a shared_ptr to the
 * decoded asset (keeping it alive) and a frame index. The renderer uploads
 * all frames to the GPU once as a texture array, then selects the frame
 * via shader uniform each draw call.
 */
class Layer {
  public:
    /**
     * @param asset       The image asset (may be single-frame or animated).
     * @param frame_index Which frame of the asset to display.
     * @param z_index     Draw order; lower values drawn first (behind higher).
     */
    Layer(std::shared_ptr<ImageAsset> asset, int frame_index, uint16_t z_index);

    const std::shared_ptr<ImageAsset>& get_asset() const { return asset; }
    int get_frame_index() const { return frame_index; }
    uint16_t get_z_index() const { return z_index; }

  private:
    std::shared_ptr<ImageAsset> asset;
    int frame_index;
    uint16_t z_index;
};

/**
 * @brief An ordered collection of Layers, keyed by integer identifier.
 */
class LayerGroup {
  public:
    LayerGroup();

    void add_layer(int id, Layer layer);
    const std::map<int, Layer>& get_layers() const { return layers; }

  private:
    std::map<int, Layer> layers;
};
