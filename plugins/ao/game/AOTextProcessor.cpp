#include "ao/game/AOTextProcessor.h"
#include "utils/UTF8.h"

#include <algorithm>

ProcessedText AOTextProcessor::process(const std::string& raw, const std::vector<AOTextColorDef>& colors,
                                       int base_color) {
    ProcessedText result;
    result.alignment = TextAlignment::LEFT;

    if (raw.empty())
        return result;

    size_t pos = 0;

    // --- Alignment prefix (only at start) ---
    if (raw.size() >= 2) {
        if (raw[0] == '~' && raw[1] == '~') {
            result.alignment = TextAlignment::CENTER;
            pos = 2;
        }
        else if (raw[0] == '~' && raw[1] == '>') {
            result.alignment = TextAlignment::RIGHT;
            pos = 2;
        }
        else if (raw[0] == '<' && raw[1] == '>') {
            result.alignment = TextAlignment::JUSTIFY;
            pos = 2;
        }
    }

    // Track active inline color (-1 = using base_color, otherwise the inline color index)
    int active_inline_color = -1;
    int display_chars = 0;

    while (pos < raw.size()) {
        char c = raw[pos];

        // --- Backslash escape sequences ---
        if (c == '\\' && pos + 1 < raw.size()) {
            char next = raw[pos + 1];
            switch (next) {
            case 'n':
                // Insert actual newline into display text
                result.display += '\n';
                display_chars++;
                pos += 2;
                continue;
            case 's':
                result.events.push_back({display_chars, TextEventType::SCREENSHAKE});
                pos += 2;
                continue;
            case 'f':
                result.events.push_back({display_chars, TextEventType::FLASH});
                pos += 2;
                continue;
            case 'p':
                result.events.push_back({display_chars, TextEventType::PAUSE});
                pos += 2;
                continue;
            default:
                // Escaped literal — append the character after backslash
                pos++; // skip backslash
                break; // fall through to normal character handling
            }
        }

        // --- Speed modifiers ---
        if (c == '{') {
            result.events.push_back({display_chars, TextEventType::SPEED_DOWN});
            pos++;
            continue;
        }
        if (c == '}') {
            result.events.push_back({display_chars, TextEventType::SPEED_UP});
            pos++;
            continue;
        }

        // --- Color markdown ---
        bool matched_color = false;
        for (int ci = 0; ci < (int)colors.size(); ci++) {
            const auto& color = colors[ci];
            if (color.markup_start.empty())
                continue;

            bool is_toggle = color.markup_end.empty() || color.markup_end == color.markup_start;

            if (raw.compare(pos, color.markup_start.size(), color.markup_start) == 0) {
                if (active_inline_color == ci && is_toggle) {
                    // Toggle OFF — revert to base color
                    active_inline_color = -1;
                    result.events.push_back({display_chars, TextEventType::COLOR_CHANGE, base_color});
                }
                else if (active_inline_color != ci) {
                    // Enter this color
                    active_inline_color = ci;
                    result.events.push_back({display_chars, TextEventType::COLOR_CHANGE, ci});
                }
                if (color.markup_remove) {
                    pos += color.markup_start.size();
                    matched_color = true;
                    break;
                }
            }
            // Explicit end marker (non-toggle mode)
            else if (!is_toggle && active_inline_color == ci &&
                     raw.compare(pos, color.markup_end.size(), color.markup_end) == 0) {
                active_inline_color = -1;
                result.events.push_back({display_chars, TextEventType::COLOR_CHANGE, base_color});
                if (color.markup_remove) {
                    pos += color.markup_end.size();
                    matched_color = true;
                    break;
                }
            }
        }
        if (matched_color)
            continue;

        // --- Normal character (UTF-8 aware) ---
        // Decode one UTF-8 codepoint and append all its bytes
        size_t start = pos;
        UTF8::decode(raw, pos); // advances pos past the codepoint
        result.display.append(raw, start, pos - start);
        display_chars++;
    }

    return result;
}

void AOTextProcessor::apply_alignment(std::vector<TextRenderer::GlyphLayout>& layout, TextAlignment align,
                                      int wrap_width) {
    if (layout.empty() || align == TextAlignment::LEFT || wrap_width <= 0)
        return;

    // Group glyphs into lines by pen_y. Find each line's extent and shift.
    size_t line_start = 0;
    while (line_start < layout.size()) {
        int line_y = layout[line_start].pen_y;

        // Find end of this line
        size_t line_end = line_start + 1;
        while (line_end < layout.size() && layout[line_end].pen_y == line_y)
            line_end++;

        // Find the rightmost pixel on this line (pen_x of last glyph + approximate advance)
        // We use the last glyph's pen_x as the line width proxy. For accurate width,
        // we'd need glyph advances, but pen_x of the last char + a small estimate works.
        int line_max_x = layout[line_end - 1].pen_x;

        // Estimate line width: last glyph pen_x + average char width (~8px)
        // This is imprecise but sufficient for alignment since compute_layout
        // already positions glyphs with proper advances.
        int line_width = line_max_x + 8; // approximate advance of last glyph
        int slack = wrap_width - line_width;
        if (slack <= 0) {
            line_start = line_end;
            continue;
        }

        int shift = 0;
        if (align == TextAlignment::CENTER) {
            shift = slack / 2;
        }
        else if (align == TextAlignment::RIGHT) {
            shift = slack;
        }
        else if (align == TextAlignment::JUSTIFY) {
            // Justify: distribute slack evenly between words (space gaps).
            // Count spaces on this line.
            int space_count = 0;
            for (size_t i = line_start; i < line_end; i++) {
                if (layout[i].codepoint == ' ')
                    space_count++;
            }

            // Don't justify the last line (standard typographic convention)
            bool is_last_line = (line_end >= layout.size());
            if (space_count > 0 && !is_last_line) {
                int extra_per_space = slack / space_count;
                int remainder = slack % space_count;
                int accumulated = 0;
                int space_idx = 0;
                for (size_t i = line_start; i < line_end; i++) {
                    layout[i].pen_x += accumulated;
                    if (layout[i].codepoint == ' ') {
                        accumulated += extra_per_space + (space_idx < remainder ? 1 : 0);
                        space_idx++;
                    }
                }
            }
            line_start = line_end;
            continue; // justify already applied
        }

        // Apply uniform shift (center/right)
        for (size_t i = line_start; i < line_end; i++)
            layout[i].pen_x += shift;

        line_start = line_end;
    }
}
