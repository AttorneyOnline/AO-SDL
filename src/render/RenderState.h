#ifndef RENDERSTATE_H
#define RENDERSTATE_H

#include "Layer.h"

#include <map>

class RenderState {
  public:
    RenderState();

    void add_layer_group(int id, LayerGroup layer_group);
    LayerGroup get_layer_group(int id);

    const std::map<int, LayerGroup> get_layer_groups() const;

  private:
    std::map<int, LayerGroup> layer_groups;
};

#endif
