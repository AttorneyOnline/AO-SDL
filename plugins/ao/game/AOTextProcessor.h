#pragma once

#include "ao/asset/AOAssetLibrary.h"
#include "render/TextRenderer.h"

#include <string>
#include <vector>

/// Inline text event triggered at a specific character position.
enum class TextEventType {
    COLOR_CHANGE, ///< Switch to a different color index.
    SPEED_UP,     ///< Faster text crawl (decrement speed index).
    SPEED_DOWN,   ///< Slower text crawl (increment speed index).
    PAUSE,        ///< 100ms pause in text crawl.
    SCREENSHAKE,  ///< Trigger screen shake effect.
    FLASH,        ///< Trigger flash/realization effect.
};

struct TextEvent {
    int char_index; ///< Display-string character position (0-based).
    TextEventType type;
    int color_idx = 0; ///< Only meaningful for COLOR_CHANGE.
};

/// Message-level text alignment (prefix at start of raw message).
enum class TextAlignment { LEFT, CENTER, RIGHT, JUSTIFY };

/// Result of preprocessing an IC message string.
struct ProcessedText {
    std::string display;           ///< Clean text with markup stripped.
    std::vector<TextEvent> events; ///< Sorted by char_index ascending.
    TextAlignment alignment = TextAlignment::LEFT;
};

/// Result returned from AOTextBox::tick() to signal inline effects.
struct TickResult {
    bool advanced = false;            ///< Whether new characters became visible.
    bool trigger_screenshake = false; ///< Inline \s was reached.
    bool trigger_flash = false;       ///< Inline \f was reached.
};

/**
 * @brief Preprocesses AO2 IC message text, stripping inline markup and
 *        producing a list of events keyed by display-character position.
 *
 * Supported markup:
 *   - Escape sequences: \n (newline), \s (screenshake), \f (flash), \p (pause)
 *   - Speed modifiers: { (slower), } (faster)
 *   - Alignment prefixes: ~~ (center), ~> (right), <> (justify)
 *   - Color markdown: configurable per-color start/end characters from chat_config.ini
 */
class AOTextProcessor {
  public:
    /// Process a raw IC message string into clean display text + events.
    static ProcessedText process(const std::string& raw, const std::vector<AOTextColorDef>& colors, int base_color);

    /// Apply text alignment to a pre-computed glyph layout.
    /// Modifies pen_x values in-place. Glyphs are grouped into lines by pen_y.
    static void apply_alignment(std::vector<TextRenderer::GlyphLayout>& layout, TextAlignment align, int wrap_width);
};
