#pragma once
#include <cstdint>
#include <string>

namespace UTF8 {

/// Decode one codepoint from s at pos, advance pos. Returns 0xFFFD on error.
inline uint32_t decode(const std::string& s, size_t& pos) {
    if (pos >= s.size())
        return 0;
    uint8_t c = s[pos];
    uint32_t cp;
    int extra;
    if (c < 0x80) {
        cp = c;
        extra = 0;
    }
    else if ((c & 0xE0) == 0xC0) {
        cp = c & 0x1F;
        extra = 1;
    }
    else if ((c & 0xF0) == 0xE0) {
        cp = c & 0x0F;
        extra = 2;
    }
    else if ((c & 0xF8) == 0xF0) {
        cp = c & 0x07;
        extra = 3;
    }
    else {
        pos++;
        return 0xFFFD;
    }
    pos++;
    for (int i = 0; i < extra && pos < s.size(); i++, pos++)
        cp = (cp << 6) | (s[pos] & 0x3F);
    return cp;
}

/// Count UTF-8 codepoints in a string.
inline int length(const std::string& s) {
    int count = 0;
    size_t pos = 0;
    while (pos < s.size()) {
        decode(s, pos);
        count++;
    }
    return count;
}

/// Get the byte offset of the Nth character (0-indexed).
inline size_t byte_offset(const std::string& s, int char_idx) {
    size_t pos = 0;
    for (int i = 0; i < char_idx && pos < s.size(); i++)
        decode(s, pos);
    return pos;
}

} // namespace UTF8
