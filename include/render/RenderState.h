/**
 * @file RenderState.h
 * @brief Describes a complete frame as an ordered map of LayerGroups.
 */
#pragma once

#include "Layer.h"

#include <map>

/**
 * @brief A snapshot of the scene to be rendered, organized as a map of LayerGroups.
 *
 * Each LayerGroup is keyed by an integer identifier and contains the layers
 * that belong to that group. The map ordering determines the draw order
 * of groups.
 */
class RenderState {
  public:
    /** @brief Default-construct an empty RenderState. */
    RenderState();

    /**
     * @brief Add or replace a LayerGroup.
     * @param id    Unique identifier for the layer group.
     * @param layer_group The LayerGroup to insert.
     */
    void add_layer_group(int id, LayerGroup layer_group);

    /**
     * @brief Retrieve a LayerGroup by its identifier.
     * @param id Identifier of the requested group.
     * @return A copy of the LayerGroup associated with @p id.
     */
    LayerGroup get_layer_group(int id);

    /**
     * @brief Get all layer groups.
     * @return A const copy of the internal map of LayerGroups keyed by id.
     */
    const std::map<int, LayerGroup> get_layer_groups() const;

  private:
    std::map<int, LayerGroup> layer_groups; ///< All layer groups in this state.
};
