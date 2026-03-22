#include "render/TextMeshBuilder.h"

namespace TextMeshBuilder {

void build(GlyphCache& cache, const std::vector<TextRenderer::GlyphLayout>& layout, int chars_visible, int offset_x,
           int offset_y, int scroll_y, int max_height, int base_w, int base_h, std::vector<MeshVertex>& out_vertices,
           std::vector<uint32_t>& out_indices) {
    out_vertices.clear();
    out_indices.clear();

    float atlas_w = (float)cache.atlas_width();
    float atlas_h = (float)cache.atlas_height();
    float bw = (float)base_w;
    float bh = (float)base_h;
    int ascender = cache.ascender();

    for (const auto& gl : layout) {
        if (gl.char_index >= chars_visible)
            break;

        const GlyphInfo& info = cache.ensure(gl.codepoint);
        if (info.width == 0 || info.height == 0)
            continue; // space or empty glyph

        // Pixel position of this glyph
        float px = (float)(offset_x + gl.pen_x + info.bearing_x);
        float py = (float)(offset_y + ascender + gl.pen_y - scroll_y - info.bearing_y);
        float pw = (float)info.width;
        float ph = (float)info.height;

        // Clip against visible text area
        float clip_top = (float)offset_y;
        float clip_bottom = (max_height > 0) ? (float)(offset_y + max_height) : bh;

        // Skip glyphs entirely outside
        if (py + ph < clip_top || py > clip_bottom)
            continue;

        // UV in atlas (stored bottom-up: glyph top = high V, glyph bottom = low V)
        float u0 = (float)info.atlas_x / atlas_w;
        float u1 = (float)(info.atlas_x + info.width) / atlas_w;
        float v_top = (float)(info.atlas_y + info.height) / atlas_h;
        float v_bot = (float)info.atlas_y / atlas_h;

        // Clamp partially visible glyphs at clip boundaries
        float v_range = v_bot - v_top;
        float orig_ph = ph;
        if (py < clip_top) {
            float clipped = clip_top - py;
            v_top += (clipped / orig_ph) * v_range;
            ph -= clipped;
            py = clip_top;
        }
        if (py + ph > clip_bottom) {
            float clipped = (py + ph) - clip_bottom;
            v_bot -= (clipped / orig_ph) * v_range;
            ph -= clipped;
        }

        // Convert pixel coords to NDC [-1, 1]
        float x0 = (px / bw) * 2.0f - 1.0f;
        float y0 = 1.0f - (py / bh) * 2.0f; // top (Y flipped for GL)
        float x1 = ((px + pw) / bw) * 2.0f - 1.0f;
        float y1 = 1.0f - ((py + ph) / bh) * 2.0f; // bottom

        uint32_t base = (uint32_t)out_vertices.size();

        out_vertices.push_back({{x0, y0}, {u0, v_top}}); // top-left
        out_vertices.push_back({{x1, y0}, {u1, v_top}}); // top-right
        out_vertices.push_back({{x1, y1}, {u1, v_bot}}); // bottom-right
        out_vertices.push_back({{x0, y1}, {u0, v_bot}}); // bottom-left

        out_indices.push_back(base + 0);
        out_indices.push_back(base + 1);
        out_indices.push_back(base + 3);
        out_indices.push_back(base + 1);
        out_indices.push_back(base + 2);
        out_indices.push_back(base + 3);
    }
}

} // namespace TextMeshBuilder
