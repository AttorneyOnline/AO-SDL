#include "render/GlyphCache.h"

#include "utils/Log.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cstring>

// Access FreeType internals from TextRenderer via its Impl.
// TextRenderer exposes load_char() which loads + renders a glyph into the
// FT_Face's glyph slot. We read the bitmap from there.
//
// This is a deliberate coupling: GlyphCache and TextRenderer share the
// same FreeType faces. The alternative (duplicating FT_Face ownership)
// would double memory for font data.

GlyphCache::GlyphCache(TextRenderer& renderer, int atlas_size)
    : renderer_(renderer), atlas_w_(atlas_size), atlas_h_(atlas_size) {
    atlas_pixels_.resize((size_t)atlas_w_ * atlas_h_ * 4, 0);

    ImageFrame frame;
    frame.width = atlas_w_;
    frame.height = atlas_h_;
    frame.duration_ms = 0;
    frame.pixels = atlas_pixels_;

    atlas_ = std::make_shared<ImageAsset>("_glyph_atlas", "gpu", std::vector<ImageFrame>{std::move(frame)});

    // Precache printable ASCII (space through tilde)
    for (uint32_t cp = 0x20; cp <= 0x7E; cp++)
        ensure(cp);

    Log::log_print(DEBUG, "GlyphCache: created %dx%d atlas, precached ASCII", atlas_w_, atlas_h_);
}

void GlyphCache::grow_atlas() {
    int new_h = std::min(atlas_h_ * 2, 2048);
    if (new_h == atlas_h_) {
        Log::log_print(WARNING, "GlyphCache: atlas at max size %dx%d, cannot grow", atlas_w_, atlas_h_);
        return;
    }

    std::vector<uint8_t> new_pixels((size_t)atlas_w_ * new_h * 4, 0);
    std::memcpy(new_pixels.data(), atlas_pixels_.data(), atlas_pixels_.size());
    atlas_pixels_ = std::move(new_pixels);
    atlas_h_ = new_h;

    atlas_->update_frame(0, atlas_pixels_);
    Log::log_print(DEBUG, "GlyphCache: grew atlas to %dx%d", atlas_w_, atlas_h_);
}

const GlyphInfo& GlyphCache::ensure(uint32_t codepoint) {
    auto it = cache_.find(codepoint);
    if (it != cache_.end())
        return it->second;

    // Render the glyph via TextRenderer's FreeType pipeline.
    // We use render() with a tiny 1-glyph string to get the bitmap.
    // This is a bit indirect, but it reuses the fallback chain logic.
    //
    // Actually, we need direct FreeType access. TextRenderer::render()
    // blits into a pixel buffer, but we need the raw glyph bitmap.
    // Since compute_layout() + load_char() are the public API, and
    // load_char() loads the glyph into the FT_Face's glyph slot,
    // we can read the bitmap after that call.
    //
    // But load_char is on Impl which is private. Instead, we use
    // render() into a small buffer and extract the glyph that way.
    //
    // Simpler approach: render the single character into a tight buffer
    // and use the entire buffer as the glyph bitmap.

    // Measure the glyph via compute_layout (single char string)
    std::string ch;
    if (codepoint < 0x80) {
        ch = (char)codepoint;
    }
    else if (codepoint < 0x800) {
        ch += (char)(0xC0 | (codepoint >> 6));
        ch += (char)(0x80 | (codepoint & 0x3F));
    }
    else if (codepoint < 0x10000) {
        ch += (char)(0xE0 | (codepoint >> 12));
        ch += (char)(0x80 | ((codepoint >> 6) & 0x3F));
        ch += (char)(0x80 | (codepoint & 0x3F));
    }
    else {
        ch += (char)(0xF0 | (codepoint >> 18));
        ch += (char)(0x80 | ((codepoint >> 12) & 0x3F));
        ch += (char)(0x80 | ((codepoint >> 6) & 0x3F));
        ch += (char)(0x80 | (codepoint & 0x3F));
    }

    auto layout = renderer_.compute_layout(ch, 0);
    int advance = renderer_.measure_width(ch);
    int line_h = renderer_.line_height();
    int asc = renderer_.ascender();

    // Render into a tight buffer
    int buf_w = std::max(advance, 1);
    int buf_h = line_h;
    std::vector<uint8_t> buf(buf_w * buf_h * 4, 0);
    TextColor white = {255, 255, 255};
    renderer_.render(ch, 1, white, 0, 0, buf_w, buf_h, 0, 0, buf.data());

    // Find actual glyph bounds (non-zero alpha)
    int min_x = buf_w, min_y = buf_h, max_x = 0, max_y = 0;
    for (int y = 0; y < buf_h; y++) {
        for (int x = 0; x < buf_w; x++) {
            if (buf[(y * buf_w + x) * 4 + 3] > 0) {
                min_x = std::min(min_x, x);
                min_y = std::min(min_y, y);
                max_x = std::max(max_x, x);
                max_y = std::max(max_y, y);
            }
        }
    }

    GlyphInfo info;
    info.advance_x = advance;

    if (max_x < min_x) {
        // Empty glyph (space, etc.) — no bitmap to pack
        info.width = 0;
        info.height = 0;
        info.bearing_x = 0;
        info.bearing_y = asc;
        cache_[codepoint] = info;
        return cache_[codepoint];
    }

    int gw = max_x - min_x + 1;
    int gh = max_y - min_y + 1;
    info.width = gw;
    info.height = gh;
    info.bearing_x = min_x;
    info.bearing_y = asc - min_y; // bitmap_top equivalent

    // Pack into atlas
    if (cursor_x_ + gw + GLYPH_PAD > atlas_w_) {
        // Next row
        cursor_x_ = 0;
        cursor_y_ += row_height_ + GLYPH_PAD;
        row_height_ = 0;
    }
    if (cursor_y_ + gh + GLYPH_PAD > atlas_h_) {
        grow_atlas();
    }

    info.atlas_x = cursor_x_;
    info.atlas_y = cursor_y_;

    // Copy glyph pixels into atlas (RGBA, alpha in A channel, RGB=255).
    // Atlas is stored bottom-up (row 0 = bottom) to match all other textures.
    // Glyph rows are flipped during copy: source top row → atlas bottom row.
    for (int y = 0; y < gh; y++) {
        int flipped_y = gh - 1 - y;
        for (int x = 0; x < gw; x++) {
            int src_idx = ((min_y + y) * buf_w + (min_x + x)) * 4;
            int dst_idx = ((info.atlas_y + flipped_y) * atlas_w_ + (info.atlas_x + x)) * 4;
            atlas_pixels_[dst_idx + 0] = 255;
            atlas_pixels_[dst_idx + 1] = 255;
            atlas_pixels_[dst_idx + 2] = 255;
            atlas_pixels_[dst_idx + 3] = buf[src_idx + 3]; // alpha from rendered glyph
        }
    }

    cursor_x_ += gw + GLYPH_PAD;
    row_height_ = std::max(row_height_, gh);

    // Update atlas texture (stored bottom-up, matching all other textures)
    atlas_->update_frame(0, atlas_pixels_);

    cache_[codepoint] = info;
    return cache_[codepoint];
}
