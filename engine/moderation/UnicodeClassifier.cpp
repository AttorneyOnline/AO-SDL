#include "moderation/UnicodeClassifier.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

namespace moderation {

namespace {

/// Decode the next UTF-8 codepoint at `s[i]`. On success, writes the
/// codepoint to @p out and returns the number of bytes consumed.
/// On decode failure, returns 0 and leaves @p out untouched.
int decode_utf8(std::string_view s, size_t i, uint32_t& out) {
    if (i >= s.size())
        return 0;
    auto b0 = static_cast<uint8_t>(s[i]);
    if (b0 < 0x80) {
        out = b0;
        return 1;
    }
    if ((b0 & 0xE0) == 0xC0) {
        if (i + 1 >= s.size())
            return 0;
        auto b1 = static_cast<uint8_t>(s[i + 1]);
        if ((b1 & 0xC0) != 0x80)
            return 0;
        out = ((b0 & 0x1Fu) << 6) | (b1 & 0x3Fu);
        return 2;
    }
    if ((b0 & 0xF0) == 0xE0) {
        if (i + 2 >= s.size())
            return 0;
        auto b1 = static_cast<uint8_t>(s[i + 1]);
        auto b2 = static_cast<uint8_t>(s[i + 2]);
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80)
            return 0;
        out = ((b0 & 0x0Fu) << 12) | ((b1 & 0x3Fu) << 6) | (b2 & 0x3Fu);
        return 3;
    }
    if ((b0 & 0xF8) == 0xF0) {
        if (i + 3 >= s.size())
            return 0;
        auto b1 = static_cast<uint8_t>(s[i + 1]);
        auto b2 = static_cast<uint8_t>(s[i + 2]);
        auto b3 = static_cast<uint8_t>(s[i + 3]);
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80)
            return 0;
        out = ((b0 & 0x07u) << 18) | ((b1 & 0x3Fu) << 12) | ((b2 & 0x3Fu) << 6) | (b3 & 0x3Fu);
        return 4;
    }
    return 0;
}

/// Broad script categories. Designed for speed, not precision: we only
/// need to answer "is this an unusual script for a game chat server?"
/// and "how many different scripts are mixed in one message?"
enum class Script : uint8_t {
    OTHER = 0,
    ASCII,      ///< Basic latin. Always common, always fine.
    LATIN,      ///< Latin + latin extended, common.
    CYRILLIC,   ///< Common. Mixing with Latin raises a flag.
    GREEK,
    HEBREW,
    ARABIC,
    DEVANAGARI, ///< Plus related Brahmic scripts.
    CJK,        ///< Unified CJK + kana + hangul + compat ideographs.
    COMMON,     ///< Punctuation, symbols, digits — belongs to any run.
    EXOTIC,     ///< Cuneiform, Tags, Emoji Tags, rare CJK extensions, etc.
};

/// Classify a codepoint into a broad script. This is a hand-written
/// range table; it is intentionally coarse. "EXOTIC" is a grab-bag for
/// everything we consider unusual enough to penalize as visual noise.
Script classify_script(uint32_t cp) {
    if (cp < 0x80) {
        // ASCII control chars (except tab/newline) count as exotic.
        if (cp < 0x20 && cp != '\t' && cp != '\n' && cp != '\r')
            return Script::EXOTIC;
        return Script::ASCII;
    }

    // Latin extended-A / -B / IPA / spacing mods / combining diacritics.
    if (cp >= 0x0080 && cp <= 0x024F)
        return Script::LATIN;
    if (cp >= 0x1E00 && cp <= 0x1EFF)
        return Script::LATIN; // Latin extended additional
    if (cp >= 0xFF00 && cp <= 0xFF5E)
        return Script::LATIN; // Fullwidth forms

    if (cp >= 0x0370 && cp <= 0x03FF)
        return Script::GREEK;
    if (cp >= 0x0400 && cp <= 0x04FF)
        return Script::CYRILLIC;
    if (cp >= 0x0500 && cp <= 0x052F)
        return Script::CYRILLIC;

    if (cp >= 0x0590 && cp <= 0x05FF)
        return Script::HEBREW;
    if (cp >= 0x0600 && cp <= 0x06FF)
        return Script::ARABIC;
    if (cp >= 0x0750 && cp <= 0x077F)
        return Script::ARABIC;
    if (cp >= 0x08A0 && cp <= 0x08FF)
        return Script::ARABIC;
    if (cp >= 0xFB50 && cp <= 0xFDFF)
        return Script::ARABIC; // Arabic presentation forms A
    if (cp >= 0xFE70 && cp <= 0xFEFF)
        return Script::ARABIC; // Arabic presentation forms B

    if (cp >= 0x0900 && cp <= 0x0DFF)
        return Script::DEVANAGARI; // Devanagari + related

    // Common: punctuation, symbols, digits, currency, etc.
    if (cp >= 0x2000 && cp <= 0x206F)
        return Script::COMMON; // General punctuation
    if (cp >= 0x2070 && cp <= 0x209F)
        return Script::COMMON; // Super/subscripts
    if (cp >= 0x20A0 && cp <= 0x20CF)
        return Script::COMMON; // Currency
    if (cp >= 0x2100 && cp <= 0x214F)
        return Script::COMMON; // Letterlike
    if (cp >= 0x2190 && cp <= 0x21FF)
        return Script::COMMON; // Arrows
    if (cp >= 0x2200 && cp <= 0x22FF)
        return Script::COMMON; // Math
    if (cp >= 0x2300 && cp <= 0x23FF)
        return Script::COMMON; // Misc technical
    if (cp >= 0x2500 && cp <= 0x259F)
        return Script::COMMON; // Box drawing
    if (cp >= 0x25A0 && cp <= 0x25FF)
        return Script::COMMON; // Geometric shapes
    if (cp >= 0x2600 && cp <= 0x26FF)
        return Script::COMMON; // Misc symbols (weather, astrological)
    if (cp >= 0x2700 && cp <= 0x27BF)
        return Script::COMMON; // Dingbats

    // Emoji and derivatives.
    if (cp >= 0x1F300 && cp <= 0x1FAFF)
        return Script::COMMON; // Emoji blocks

    // CJK unified + compat + kana + hangul.
    if (cp >= 0x3040 && cp <= 0x309F)
        return Script::CJK; // Hiragana
    if (cp >= 0x30A0 && cp <= 0x30FF)
        return Script::CJK; // Katakana
    if (cp >= 0x3400 && cp <= 0x4DBF)
        return Script::CJK; // CJK Ext A
    if (cp >= 0x4E00 && cp <= 0x9FFF)
        return Script::CJK; // CJK Unified
    if (cp >= 0xAC00 && cp <= 0xD7AF)
        return Script::CJK; // Hangul syllables
    if (cp >= 0xF900 && cp <= 0xFAFF)
        return Script::CJK; // CJK compatibility ideographs

    // Exotic: cuneiform, hieroglyphs, rare CJK extensions, tags.
    if (cp >= 0x10000 && cp <= 0x1007F)
        return Script::EXOTIC; // Linear B
    if (cp >= 0x10380 && cp <= 0x1039F)
        return Script::EXOTIC; // Ugaritic
    if (cp >= 0x10400 && cp <= 0x1044F)
        return Script::EXOTIC; // Deseret
    if (cp >= 0x12000 && cp <= 0x123FF)
        return Script::EXOTIC; // Cuneiform
    if (cp >= 0x12400 && cp <= 0x1247F)
        return Script::EXOTIC; // Cuneiform numbers
    if (cp >= 0x13000 && cp <= 0x1342F)
        return Script::EXOTIC; // Egyptian hieroglyphs
    if (cp >= 0xE0000 && cp <= 0xE007F)
        return Script::EXOTIC; // Tags
    if (cp >= 0x20000 && cp <= 0x2FFFF)
        return Script::EXOTIC; // CJK Ext B-F (very rare in normal chat)
    if (cp >= 0x30000 && cp <= 0x3FFFF)
        return Script::EXOTIC; // CJK Ext G+

    return Script::OTHER;
}

/// Is @p cp a combining mark (general category Mn, Mc, or Me)?
/// Hand-written ranges cover the common zalgo-producing blocks.
bool is_combining_mark(uint32_t cp) {
    if (cp >= 0x0300 && cp <= 0x036F) // Combining diacritical marks
        return true;
    if (cp >= 0x0483 && cp <= 0x0489) // Cyrillic combining
        return true;
    if (cp >= 0x0591 && cp <= 0x05BD) // Hebrew combining
        return true;
    if (cp >= 0x0610 && cp <= 0x061A) // Arabic combining
        return true;
    if (cp >= 0x064B && cp <= 0x065F)
        return true;
    if (cp == 0x0670)
        return true;
    if (cp >= 0x06D6 && cp <= 0x06DC)
        return true;
    if (cp >= 0x06DF && cp <= 0x06E4)
        return true;
    if (cp >= 0x06E7 && cp <= 0x06E8)
        return true;
    if (cp >= 0x06EA && cp <= 0x06ED)
        return true;
    if (cp >= 0x0900 && cp <= 0x0903) // Devanagari
        return true;
    if (cp >= 0x093A && cp <= 0x094F)
        return true;
    if (cp >= 0x0951 && cp <= 0x0957)
        return true;
    if (cp >= 0x0962 && cp <= 0x0963)
        return true;
    if (cp >= 0x1AB0 && cp <= 0x1AFF) // Combining diacritical marks extended
        return true;
    if (cp >= 0x1DC0 && cp <= 0x1DFF) // Combining diacritical marks supplement
        return true;
    if (cp >= 0x20D0 && cp <= 0x20FF) // Combining diacritical marks for symbols
        return true;
    if (cp >= 0xFE20 && cp <= 0xFE2F) // Combining half marks
        return true;
    return false;
}

/// Is @p cp a format character (general category Cf)? These are the
/// invisible directionality / joiner / separator characters that get
/// abused for homoglyph and RTL-override attacks.
bool is_format_char(uint32_t cp) {
    if (cp == 0x00AD) // Soft hyphen
        return true;
    if (cp >= 0x0600 && cp <= 0x0605) // Arabic number signs
        return true;
    if (cp == 0x061C) // Arabic letter mark
        return true;
    if (cp == 0x06DD)
        return true;
    if (cp == 0x070F)
        return true;
    if (cp == 0x180E)
        return true;
    if (cp >= 0x200B && cp <= 0x200F) // ZWSP, ZWNJ, ZWJ, LRM, RLM
        return true;
    if (cp >= 0x202A && cp <= 0x202E) // LRE, RLE, PDF, LRO, RLO
        return true;
    if (cp >= 0x2060 && cp <= 0x2064) // Word joiner, function application, etc.
        return true;
    if (cp >= 0x2066 && cp <= 0x2069) // Directional isolates
        return true;
    if (cp == 0xFEFF) // BOM / ZWNBSP
        return true;
    return false;
}

/// Is @p cp in the private use area?
bool is_private_use(uint32_t cp) {
    if (cp >= 0xE000 && cp <= 0xF8FF)
        return true;
    if (cp >= 0xF0000 && cp <= 0xFFFFD)
        return true;
    if (cp >= 0x100000 && cp <= 0x10FFFD)
        return true;
    return false;
}

} // namespace

UnicodeClassification UnicodeClassifier::classify(std::string_view message) const {
    UnicodeClassification out;
    if (!cfg_.enabled || message.empty())
        return out;

    // Track which scripts we've seen. Index by Script enum ordinal.
    std::array<bool, 11> scripts_seen{};
    int invalid_bytes = 0;

    size_t i = 0;
    while (i < message.size()) {
        uint32_t cp = 0;
        int consumed = decode_utf8(message, i, cp);
        if (consumed == 0) {
            ++invalid_bytes;
            ++i;
            continue;
        }
        i += static_cast<size_t>(consumed);
        ++out.total_codepoints;

        if (is_combining_mark(cp))
            ++out.combining_marks;
        if (is_format_char(cp))
            ++out.format_chars;
        if (is_private_use(cp))
            ++out.private_use;

        Script s = classify_script(cp);
        if (s == Script::EXOTIC)
            ++out.exotic_script;
        // COMMON and OTHER are not interesting for script-mix detection.
        if (s != Script::COMMON && s != Script::OTHER)
            scripts_seen[static_cast<size_t>(s)] = true;
    }

    for (bool seen : scripts_seen)
        if (seen)
            ++out.script_count;

    if (out.total_codepoints == 0) {
        // All bytes invalid — treat as maximally noisy.
        if (invalid_bytes > 0) {
            out.score = cfg_.max_score;
            out.reason = "invalid utf-8";
        }
        return out;
    }

    const double total = static_cast<double>(out.total_codepoints);
    const double combining_ratio = static_cast<double>(out.combining_marks) / total;
    const double exotic_ratio = static_cast<double>(out.exotic_script) / total;
    const double format_ratio = static_cast<double>(out.format_chars + out.private_use) / total;

    // Independent contributions — a message that is both zalgo-y and
    // full of exotic scripts scores higher than one that is just one.
    double score = 0.0;

    if (combining_ratio > cfg_.combining_mark_threshold) {
        score += (combining_ratio - cfg_.combining_mark_threshold) /
                 std::max(0.01, 1.0 - cfg_.combining_mark_threshold);
        out.reason += "zalgo ";
    }
    if (exotic_ratio > cfg_.exotic_script_threshold) {
        score += (exotic_ratio - cfg_.exotic_script_threshold) /
                 std::max(0.01, 1.0 - cfg_.exotic_script_threshold);
        out.reason += "exotic_script ";
    }
    if (format_ratio > cfg_.format_char_threshold) {
        score += (format_ratio - cfg_.format_char_threshold) /
                 std::max(0.01, 1.0 - cfg_.format_char_threshold);
        out.reason += "format_chars ";
    }

    // Script mixing: 3+ distinct non-common scripts in one message
    // is suspicious, unless it's a very long message where some
    // diversity is expected. We penalize moderately.
    if (out.script_count >= 3 && out.total_codepoints < 64) {
        score += 0.3;
        out.reason += "script_mix ";
    }

    // Invalid UTF-8 contributes proportionally.
    if (invalid_bytes > 0) {
        double ratio = static_cast<double>(invalid_bytes) /
                       static_cast<double>(invalid_bytes + out.total_codepoints);
        score += ratio;
        out.reason += "invalid_utf8 ";
    }

    out.score = std::clamp(score, 0.0, cfg_.max_score);
    if (!out.reason.empty() && out.reason.back() == ' ')
        out.reason.pop_back();
    return out;
}

} // namespace moderation
