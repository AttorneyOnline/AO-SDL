#include "render/RenderState.h"

RenderState::RenderState() {
}

void RenderState::add_layer_group(int id, LayerGroup layer_group) {
    layer_groups.emplace(id, std::move(layer_group));
}

LayerGroup RenderState::get_layer_group(int id) {
    return layer_groups.at(id);
}

LayerGroup* RenderState::get_mutable_layer_group(int id) {
    auto it = layer_groups.find(id);
    return it != layer_groups.end() ? &it->second : nullptr;
}

const std::map<int, LayerGroup>& RenderState::get_layer_groups() const {
    return layer_groups;
}
