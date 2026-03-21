/**
 * @file GlyphCache.h
 * @brief Per-font glyph atlas for GPU text rendering.
 *
 * Rasterizes glyphs on demand via FreeType and packs them into a single
 * RGBA atlas texture (stored as an ImageAsset). The renderer's texture
 * cache handles GPU upload automatically via generation tracking.
 */
#pragma once

#include "asset/ImageAsset.h"
#include "render/TextRenderer.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

/// Metrics and atlas location for a single cached glyph.
struct GlyphInfo {
    int atlas_x = 0, atlas_y = 0;  ///< Top-left in atlas pixels.
    int width = 0, height = 0;     ///< Glyph bitmap size.
    int bearing_x = 0, bearing_y = 0; ///< FreeType bitmap_left / bitmap_top.
    int advance_x = 0;             ///< Horizontal advance in pixels.
};

/**
 * @brief Manages a glyph atlas for a single font configuration.
 *
 * Call ensure() to guarantee a codepoint is in the atlas. The atlas
 * ImageAsset can be attached to a Layer; the renderer uploads it like
 * any other texture. Vertex data for each glyph quad is built by
 * TextMeshBuilder using the GlyphInfo metrics.
 */
class GlyphCache {
  public:
    /// Create a cache backed by an existing TextRenderer (for FreeType access).
    /// The TextRenderer must outlive this cache.
    explicit GlyphCache(TextRenderer& renderer, int atlas_size = 512);

    /// Ensure the given codepoint is in the atlas. Returns its metrics.
    /// If the glyph is new, it is rasterized and packed into the atlas.
    const GlyphInfo& ensure(uint32_t codepoint);

    /// Get the atlas as an ImageAsset (for attaching to a Layer).
    std::shared_ptr<ImageAsset> atlas_asset() const { return atlas_; }

    /// Atlas dimensions.
    int atlas_width() const { return atlas_w_; }
    int atlas_height() const { return atlas_h_; }

    /// Font metrics.
    int ascender() const { return renderer_.ascender(); }
    int line_height() const { return renderer_.line_height(); }

  private:
    void grow_atlas();

    TextRenderer& renderer_;
    std::shared_ptr<ImageAsset> atlas_;
    std::vector<uint8_t> atlas_pixels_;
    int atlas_w_, atlas_h_;

    // Row-based packing cursor
    int cursor_x_ = 0, cursor_y_ = 0;
    int row_height_ = 0;
    static constexpr int GLYPH_PAD = 1; // padding between glyphs

    std::unordered_map<uint32_t, GlyphInfo> cache_;
};
