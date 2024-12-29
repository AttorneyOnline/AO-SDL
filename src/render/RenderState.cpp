#include "RenderState.h"

RenderState::RenderState() {
}

void RenderState::add_layer_group(int id, LayerGroup layer_group) {
    layer_groups.emplace(id, layer_group);
}

LayerGroup RenderState::get_layer_group(int id) {
    return layer_groups.at(id);
}

const std::map<int, LayerGroup> RenderState::get_layer_groups() const {
    return layer_groups;
}
