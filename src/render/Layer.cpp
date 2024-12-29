#include "Layer.h"

Layer::Layer(Image image, uint16_t z_index) : image(image), z_index(z_index) {
}

Image Layer::get_image() {
    return image;
}

uint16_t Layer::get_z_index() {
    return z_index;
}

LayerGroup::LayerGroup() {
}

void LayerGroup::add_layer(const int id, Layer layer) {
    layers.emplace(id, layer);
}

const Layer LayerGroup::get_layer(const int id) {
    return layers.at(id);
}

const std::map<const int, Layer> LayerGroup::get_layers() const {
    return layers;
}
