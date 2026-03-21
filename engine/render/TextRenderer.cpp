#include "render/TextRenderer.h"

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
    bool ready = false;
    bool sharp = false;
    int ascender = 0;
    int descender = 0;
    int line_h = 0;

    ~Impl() {
        if (face)
            FT_Done_Face(face);
        if (library)
            FT_Done_FreeType(library);
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
    auto layout = compute_layout(text, 0); // no wrapping
    int max_x = 0;
    for (const auto& g : layout) {
        if (!FT_Load_Char(impl->face, g.codepoint, FT_LOAD_DEFAULT))
            max_x = std::max(max_x, g.pen_x + (int)(impl->face->glyph->advance.x >> 6));
    }
    return max_x;
}

std::vector<TextRenderer::GlyphLayout> TextRenderer::compute_layout(const std::string& text, int wrap_width) const {
    FT_Face face = impl->face;

    struct WordInfo {
        size_t byte_start;    // byte offset in text
        int char_start;       // character index
        int char_count;       // number of characters in this word
        int pixel_width;      // total advance width in pixels
        bool ends_with_break; // followed by newline or end-of-string
    };

    // Split into words (break on spaces and newlines)
    std::vector<WordInfo> words;
    {
        size_t pos = 0;
        int char_idx = 0;
        while (pos < text.size()) {
            // Skip spaces (each space is its own "word" for layout purposes)
            if (text[pos] == ' ') {
                WordInfo w;
                w.byte_start = pos;
                w.char_start = char_idx;
                w.char_count = 1;
                w.ends_with_break = false;

                size_t saved = pos;
                UTF8::decode(text, pos);
                char_idx++;

                FT_Load_Char(face, ' ', FT_LOAD_DEFAULT);
                w.pixel_width = face->glyph->advance.x >> 6;
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

            // Non-space word
            WordInfo w;
            w.byte_start = pos;
            w.char_start = char_idx;
            w.char_count = 0;
            w.pixel_width = 0;
            w.ends_with_break = false;

            while (pos < text.size() && text[pos] != ' ' && text[pos] != '\n') {
                size_t saved = pos;
                uint32_t cp = UTF8::decode(text, pos);
                if (cp == 0)
                    break;

                if (!FT_Load_Char(face, cp, FT_LOAD_DEFAULT)) {
                    w.pixel_width += face->glyph->advance.x >> 6;
                }
                w.char_count++;
                char_idx++;
            }

            words.push_back(w);
        }
    }

    // Lay out words into lines
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

        // Word wrap: if word doesn't fit on this line, move to next
        // (but don't wrap if we're at the start of a line)
        if (pen_x > 0 && pen_x + word.pixel_width > max_width) {
            pen_x = 0;
            pen_y += impl->line_h;
        }

        // Skip leading whitespace at the start of a wrapped line
        if (pen_x == 0 && word.char_count == 1 && word.byte_start < text.size() && text[word.byte_start] == ' ')
            continue;

        // If a single word is wider than the line, we need to character-break it.
        size_t pos = word.byte_start;
        for (int i = 0; i < word.char_count; i++) {
            uint32_t cp = UTF8::decode(text, pos);
            if (cp == 0)
                break;

            if (!FT_Load_Char(face, cp, FT_LOAD_DEFAULT)) {
                int advance = face->glyph->advance.x >> 6;

                // Character-level break for words wider than the line
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
    FT_Face face = impl->face;
    FT_Int32 render_flags = FT_LOAD_RENDER;
    if (impl->sharp)
        render_flags |= FT_LOAD_TARGET_MONO;

    for (auto& gl : layout) {
        if (gl.char_index >= char_count)
            break;

        if (FT_Load_Char(face, gl.codepoint, render_flags))
            continue;

        FT_GlyphSlot g = face->glyph;
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
