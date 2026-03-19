/**
 * @file Layer.h
 * @brief Layer (Image + z-index) and LayerGroup (ordered collection of Layers).
 */
#pragma once

#include "Image.h"

#include <cstdint>
#include <map>

/**
 * @brief A renderable layer consisting of an Image and a z-index for draw ordering.
 */
class Layer {
  public:
    /**
     * @brief Construct a Layer.
     * @param image   The image data for this layer.
     * @param z_index Draw order index; lower values are drawn first (behind higher values).
     */
    Layer(Image image, uint16_t z_index);

    /**
     * @brief Get the image associated with this layer.
     * @return A copy of the layer's Image.
     */
    Image get_image();

    /**
     * @brief Get the z-index of this layer.
     * @return The draw order index.
     */
    uint16_t get_z_index();

  private:
    Image image;       ///< The image data for this layer.
    uint32_t z_index;  ///< Draw order index.
};

/**
 * @brief An ordered collection of Layers, keyed by integer identifier.
 */
class LayerGroup {
  public:
    /** @brief Default-construct an empty LayerGroup. */
    LayerGroup();

    /**
     * @brief Add or replace a layer in this group.
     * @param id    Unique identifier for the layer within this group.
     * @param layer The Layer to insert.
     */
    void add_layer(const int id, Layer layer);

    /**
     * @brief Retrieve a layer by its identifier.
     * @param id Identifier of the requested layer.
     * @return A copy of the Layer associated with @p id.
     */
    const Layer get_layer(const int id);

    /**
     * @brief Get all layers in this group.
     * @return A const copy of the internal map of Layers keyed by id.
     */
    const std::map<const int, Layer> get_layers() const;

  private:
    std::map<const int, Layer> layers; ///< All layers in this group.
};
