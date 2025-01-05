#pragma once

#include "Image.h"

#include <cstdint>
#include <map>

class Layer {
  public:
    Layer(Image image, uint16_t z_index);
    Image get_image();
    uint16_t get_z_index();

  private:
    Image image;
    uint32_t z_index;
};

class LayerGroup {
  public:
    LayerGroup();

    void add_layer(const int id, Layer layer);
    const Layer get_layer(const int id);

    const std::map<const int, Layer> get_layers() const;

  private:
    std::map<const int, Layer> layers;
};
