/**
 * @file TextMeshBuilder.h
 * @brief Builds per-glyph quad meshes from text layouts for GPU rendering.
 */
#pragma once

#include "asset/MeshAsset.h"
#include "render/GlyphCache.h"
#include "render/TextRenderer.h"

#include <string>
#include <vector>

/**
 * @brief Constructs MeshAsset vertex/index data from a text layout.
 *
 * Given a GlyphCache (for atlas UVs and glyph metrics) and a layout
 * from TextRenderer::compute_layout(), produces per-glyph quads in
 * NDC coordinates, ready to be rendered with the glyph atlas texture.
 */
namespace TextMeshBuilder {

/// Per-character color for text rendering. Index corresponds to char_index in the layout.
struct CharColor {
    float r = 1.0f, g = 1.0f, b = 1.0f;
};

/// Build vertex/index data for the given text layout.
/// Positions are in NDC [-1,1] mapped from pixel coords via base_w/base_h.
/// Only glyphs with char_index < chars_visible are emitted.
/// offset_x/offset_y are pixel offsets for the text origin (e.g. message rect position).
/// scroll_y is the vertical scroll offset in pixels (for auto-scrolling text).
/// char_colors: optional per-character colors. If empty, all vertices get white (1,1,1).
void build(GlyphCache& cache, const std::vector<TextRenderer::GlyphLayout>& layout, int chars_visible, int offset_x,
           int offset_y, int scroll_y, int max_height, int base_w, int base_h, std::vector<MeshVertex>& out_vertices,
           std::vector<uint32_t>& out_indices, const std::vector<CharColor>& char_colors = {});

} // namespace TextMeshBuilder
