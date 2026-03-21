/**
 * @file TextRenderer.h
 * @brief FreeType-backed text rasterizer for rendering into pixel buffers.
 */
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief An RGB color for text rendering.
 */
struct TextColor {
    uint8_t r, g, b;
};

/**
 * @brief Renders text into an RGBA pixel buffer using FreeType.
 *
 * Loads a font file, rasterizes glyphs at a given size, and composites
 * them into a caller-provided pixel buffer. Handles word wrapping within
 * a specified width. No GPU involvement — pure CPU rasterization.
 *
 * Thread safety: NOT thread-safe. Create one per thread if needed.
 */
class TextRenderer {
  public:
    TextRenderer();
    ~TextRenderer();

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    /**
     * @brief Load a font from a file path.
     * @param path Path to a .ttf or .otf font file.
     * @param size_px Pixel size for rendering.
     * @return true if the font loaded successfully.
     */
    bool load_font(const std::string& path, int size_px);

    /**
     * @brief Load a font from memory.
     * @param data Font file data (must remain valid for the lifetime of the TextRenderer).
     * @param size_px Pixel size for rendering.
     * @return true if the font loaded successfully.
     */
    bool load_font_memory(const uint8_t* data, size_t data_size, int size_px);

    /**
     * @brief Enable or disable anti-aliasing.
     *
     * When sharp=true, glyphs are rendered as 1-bit bitmaps (no smoothing),
     * matching the AO2 legacy client's "sharp" mode for pixel fonts.
     * Default is false (anti-aliased).
     */
    void set_sharp(bool sharp);

    /**
     * @brief Render a substring of text into an RGBA pixel buffer.
     *
     * Renders characters [0, char_count) of the given text, with word wrapping
     * at wrap_width. The buffer must be pre-allocated and cleared by the caller.
     *
     * @param text      Full text string (UTF-8).
     * @param char_count Number of characters to render (for scrolling effect).
     * @param color     Text color.
     * @param x         Starting X offset in the buffer.
     * @param y         Starting Y offset in the buffer.
     * @param buf_width Buffer width in pixels.
     * @param buf_height Buffer height in pixels.
     * @param wrap_width Maximum line width before wrapping (pixels). 0 = no wrap.
     * @param max_height Maximum visible text height (pixels). If the rendered text
     *                   exceeds this, it auto-scrolls so the last visible line
     *                   stays at the bottom. 0 = no scroll limit.
     * @param pixels    Output RGBA pixel buffer (buf_width * buf_height * 4 bytes).
     */
    void render(const std::string& text, int char_count, TextColor color, int x, int y, int buf_width, int buf_height,
                int wrap_width, int max_height, uint8_t* pixels);

    /** @brief Line height in pixels for the current font. */
    int line_height() const;

    /** @brief Distance from baseline to top of tallest glyph (pixels). */
    int ascender() const;

    /** @brief Distance from baseline to bottom of lowest glyph (pixels, positive). */
    int descender() const;

    /** @brief Measure the width in pixels of the given text (no wrapping). */
    int measure_width(const std::string& text) const;

    struct GlyphLayout {
        uint32_t codepoint;
        int pen_x, pen_y;
        int char_index;
    };

  private:
    std::vector<GlyphLayout> compute_layout(const std::string& text, int wrap_width) const;
    int compute_scroll_offset(const std::vector<GlyphLayout>& layout, int char_count, int max_height);
    void blit_glyphs(const std::vector<GlyphLayout>& layout, int char_count, TextColor color, int x, int y,
                     int scroll_y, int max_height, int buf_width, int buf_height, uint8_t* pixels);

    struct Impl;
    std::unique_ptr<Impl> impl;
};
