#include "render/Layer.h"

Layer::Layer(std::shared_ptr<ImageAsset> asset, int frame_index, uint16_t z_index)
    : asset(std::move(asset)), frame_index(frame_index), z_index(z_index) {}

LayerGroup::LayerGroup() {}

void LayerGroup::add_layer(int id, Layer layer) {
    layers.insert_or_assign(id, std::move(layer));
}
