#include "render/TextRenderer.h"

#include "asset/MountEmbedded.h"
#include "platform/SystemFonts.h"
#include "utils/BlendOps.h"
#include "utils/Log.h"
#include "utils/UTF8.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cstring>

struct TextRenderer::Impl {
    FT_Library library = nullptr;
    FT_Face face = nullptr;
    std::vector<FT_Face> fallbacks;
    bool ready = false;
    bool sharp = false;
    int ascender = 0;
    int descender = 0;
    int line_h = 0;

    ~Impl() {
        for (auto fb : fallbacks)
            FT_Done_Face(fb);
        if (face)
            FT_Done_Face(face);
        if (library)
            FT_Done_FreeType(library);
    }

    /// Try primary face, then each fallback in order. Returns the face
    /// that has the glyph, or falls back to .notdef on primary as last resort.
    FT_Face load_char(uint32_t codepoint, FT_Int32 flags) const {
        if (FT_Get_Char_Index(face, codepoint) != 0 && !FT_Load_Char(face, codepoint, flags))
            return face;
        for (auto fb : fallbacks) {
            if (FT_Get_Char_Index(fb, codepoint) != 0 && !FT_Load_Char(fb, codepoint, flags))
                return fb;
        }
        // Last resort: .notdef from primary
        if (!FT_Load_Char(face, codepoint, flags))
            return face;
        return nullptr;
    }

    void load_fallbacks(int size_px) {
        for (auto fb : fallbacks)
            FT_Done_Face(fb);
        fallbacks.clear();

        // Add bundled Noto Emoji FIRST so it takes priority over Apple Color
        // Emoji. Apple Color Emoji uses color bitmap tables (sbix/COLR) that
        // FreeType renders as color bitmaps — incompatible with GlyphCache's
        // grayscale alpha pipeline. Noto Emoji has standard outlines.
        for (const auto& file : embedded_assets()) {
            if (std::strcmp(file.path, "fonts/NotoEmoji.ttf") == 0) {
                FT_Face fb = nullptr;
                if (!FT_New_Memory_Face(library, file.data, static_cast<FT_Long>(file.size), 0, &fb)) {
                    FT_Set_Pixel_Sizes(fb, 0, size_px);
                    fallbacks.push_back(fb);
                    Log::log_print(VERBOSE, "TextRenderer: fallback [%zu] bundled NotoEmoji", fallbacks.size());
                }
                break;
            }
        }

        auto paths = platform::fallback_font_paths();
        for (const auto& path : paths) {
            FT_Face fb = nullptr;
            if (!FT_New_Face(library, path.c_str(), 0, &fb)) {
                FT_Set_Pixel_Sizes(fb, 0, size_px);
                fallbacks.push_back(fb);
                Log::log_print(VERBOSE, "TextRenderer: fallback [%zu] %s", fallbacks.size(), path.c_str());
            }
        }

        if (fallbacks.empty())
            Log::log_print(WARNING, "TextRenderer: no fallback fonts found");
    }
};

TextRenderer::TextRenderer() : impl(std::make_unique<Impl>()) {
    if (FT_Init_FreeType(&impl->library)) {
        Log::log_print(ERR, "TextRenderer: FreeType init failed");
        impl->library = nullptr;
    }
}

TextRenderer::~TextRenderer() = default;

bool TextRenderer::load_font(const std::string& path, int size_px) {
    if (!impl->library)
        return false;
    if (impl->face) {
        FT_Done_Face(impl->face);
        impl->face = nullptr;
    }

    if (FT_New_Face(impl->library, path.c_str(), 0, &impl->face)) {
        Log::log_print(ERR, "TextRenderer: failed to load font %s", path.c_str());
        impl->face = nullptr;
        impl->ready = false;
        return false;
    }

    FT_Set_Pixel_Sizes(impl->face, 0, size_px);
    impl->ascender = impl->face->size->metrics.ascender >> 6;
    impl->descender = -(impl->face->size->metrics.descender >> 6); // FT descender is negative
    impl->line_h = impl->face->size->metrics.height >> 6;
    impl->ready = true;
    impl->load_fallbacks(size_px);
    return true;
}

bool TextRenderer::load_font_memory(const uint8_t* data, size_t data_size, int size_px) {
    if (!impl->library)
        return false;
    if (impl->face) {
        FT_Done_Face(impl->face);
        impl->face = nullptr;
    }

    if (FT_New_Memory_Face(impl->library, data, (FT_Long)data_size, 0, &impl->face)) {
        Log::log_print(ERR, "TextRenderer: failed to load font from memory");
        impl->face = nullptr;
        impl->ready = false;
        return false;
    }

    FT_Set_Pixel_Sizes(impl->face, 0, size_px);
    impl->ascender = impl->face->size->metrics.ascender >> 6;
    impl->descender = -(impl->face->size->metrics.descender >> 6);
    impl->line_h = impl->face->size->metrics.height >> 6;
    impl->ready = true;
    impl->load_fallbacks(size_px);
    return true;
}

void TextRenderer::set_sharp(bool sharp) {
    impl->sharp = sharp;
}

int TextRenderer::line_height() const {
    return impl->ready ? impl->line_h : 0;
}

int TextRenderer::ascender() const {
    return impl->ready ? impl->ascender : 0;
}

int TextRenderer::descender() const {
    return impl->ready ? impl->descender : 0;
}

int TextRenderer::measure_width(const std::string& text) const {
    auto layout = compute_layout(text, 0);
    int max_x = 0;
    for (const auto& g : layout) {
        FT_Face f = impl->load_char(g.codepoint, FT_LOAD_DEFAULT);
        if (f)
            max_x = std::max(max_x, g.pen_x + (int)(f->glyph->advance.x >> 6));
    }
    return max_x;
}

std::vector<TextRenderer::GlyphLayout> TextRenderer::compute_layout(const std::string& text, int wrap_width) const {
    struct WordInfo {
        size_t byte_start;
        int char_start;
        int char_count;
        int pixel_width;
        bool ends_with_break;
    };

    std::vector<WordInfo> words;
    {
        size_t pos = 0;
        int char_idx = 0;
        while (pos < text.size()) {
            if (text[pos] == ' ') {
                WordInfo w;
                w.byte_start = pos;
                w.char_start = char_idx;
                w.char_count = 1;
                w.ends_with_break = false;

                UTF8::decode(text, pos);
                char_idx++;

                FT_Face f = impl->load_char(' ', FT_LOAD_DEFAULT);
                w.pixel_width = f ? (f->glyph->advance.x >> 6) : 0;
                words.push_back(w);
                continue;
            }

            if (text[pos] == '\n') {
                WordInfo w;
                w.byte_start = pos;
                w.char_start = char_idx;
                w.char_count = 1;
                w.pixel_width = 0;
                w.ends_with_break = true;
                words.push_back(w);
                pos++;
                char_idx++;
                continue;
            }

            WordInfo w;
            w.byte_start = pos;
            w.char_start = char_idx;
            w.char_count = 0;
            w.pixel_width = 0;
            w.ends_with_break = false;

            while (pos < text.size() && text[pos] != ' ' && text[pos] != '\n') {
                uint32_t cp = UTF8::decode(text, pos);
                if (cp == 0)
                    break;

                FT_Face f = impl->load_char(cp, FT_LOAD_DEFAULT);
                if (f)
                    w.pixel_width += f->glyph->advance.x >> 6;
                w.char_count++;
                char_idx++;
            }

            words.push_back(w);
        }
    }

    std::vector<GlyphLayout> layout;
    int pen_x = 0;
    int pen_y = 0;
    int max_width = (wrap_width > 0) ? wrap_width : 0x7FFFFFFF;

    for (auto& word : words) {
        if (word.ends_with_break) {
            pen_x = 0;
            pen_y += impl->line_h;
            continue;
        }

        if (pen_x > 0 && pen_x + word.pixel_width > max_width) {
            pen_x = 0;
            pen_y += impl->line_h;
        }

        if (pen_x == 0 && word.char_count == 1 && word.byte_start < text.size() && text[word.byte_start] == ' ')
            continue;

        size_t pos = word.byte_start;
        for (int i = 0; i < word.char_count; i++) {
            uint32_t cp = UTF8::decode(text, pos);
            if (cp == 0)
                break;

            FT_Face f = impl->load_char(cp, FT_LOAD_DEFAULT);
            if (f) {
                int advance = f->glyph->advance.x >> 6;

                if (pen_x > 0 && pen_x + advance > max_width) {
                    pen_x = 0;
                    pen_y += impl->line_h;
                }

                GlyphLayout gl;
                gl.codepoint = cp;
                gl.pen_x = pen_x;
                gl.pen_y = pen_y;
                gl.char_index = word.char_start + i;
                layout.push_back(gl);

                pen_x += advance;
            }
        }
    }

    return layout;
}

int TextRenderer::compute_scroll_offset(const std::vector<GlyphLayout>& layout, int char_count, int max_height) {
    if (max_height <= 0 || layout.empty())
        return 0;

    // Find the pen_y of the last glyph within char_count
    int last_line_y = 0;
    for (auto& gl : layout) {
        if (gl.char_index >= char_count)
            break;
        last_line_y = gl.pen_y;
    }
    // The bottom of the last line is at last_line_y + line_height
    int text_bottom = last_line_y + impl->line_h;
    if (text_bottom > max_height) {
        return text_bottom - max_height;
    }
    return 0;
}

void TextRenderer::blit_glyphs(const std::vector<GlyphLayout>& layout, int char_count, TextColor color, int x, int y,
                               int scroll_y, int max_height, int buf_width, int buf_height, uint8_t* pixels) {
    FT_Int32 render_flags = FT_LOAD_RENDER;
    if (impl->sharp)
        render_flags |= FT_LOAD_TARGET_MONO;

    for (auto& gl : layout) {
        if (gl.char_index >= char_count)
            break;

        FT_Face face = impl->load_char(gl.codepoint, render_flags);
        if (!face)
            continue;

        FT_GlyphSlot g = face->glyph;

        // Skip unsupported bitmap formats (e.g. Apple Color Emoji produces
        // FT_PIXEL_MODE_BGRA which is 4 bytes/pixel — our pipeline expects
        // grayscale or mono).
        if (!g->bitmap.buffer ||
            (g->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY && g->bitmap.pixel_mode != FT_PIXEL_MODE_MONO))
            continue;

        int gx = x + gl.pen_x + g->bitmap_left;
        int gy = y + impl->ascender + gl.pen_y - scroll_y - g->bitmap_top;

        // Clip bounds: horizontally to buffer, vertically to text area
        int clip_top = y;
        int clip_bottom = (max_height > 0) ? (y + max_height) : buf_height;

        for (unsigned int row = 0; row < g->bitmap.rows; row++) {
            int dy = gy + (int)row;
            if (dy < clip_top || dy >= clip_bottom || dy >= buf_height)
                continue;

            for (unsigned int col = 0; col < g->bitmap.width; col++) {
                int dx = gx + (int)col;
                if (dx < 0 || dx >= buf_width)
                    continue;

                uint8_t alpha;
                if (g->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
                    int byte_idx = col / 8;
                    int bit_idx = 7 - (col % 8);
                    alpha = (g->bitmap.buffer[row * g->bitmap.pitch + byte_idx] >> bit_idx) & 1 ? 255 : 0;
                }
                else {
                    alpha = g->bitmap.buffer[row * g->bitmap.pitch + col];
                }

                if (alpha == 0)
                    continue;

                size_t idx = ((size_t)dy * buf_width + dx) * 4;
                BlendOps::blend_color(pixels + idx, color.r, color.g, color.b, alpha);
            }
        }
    }
}

void TextRenderer::render(const std::string& text, int char_count, TextColor color, int x, int y, int buf_width,
                          int buf_height, int wrap_width, int max_height, uint8_t* pixels) {
    if (!impl->ready || text.empty() || char_count <= 0)
        return;

    auto layout = compute_layout(text, wrap_width > 0 ? wrap_width : buf_width);
    int scroll_y = compute_scroll_offset(layout, char_count, max_height);
    blit_glyphs(layout, char_count, color, x, y, scroll_y, max_height, buf_width, buf_height, pixels);
}
