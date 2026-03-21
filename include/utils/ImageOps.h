#pragma once

#include <cstdint>
#include <cstring>

/// Flip an RGBA pixel buffer vertically in-place.
inline void flip_vertical_rgba(uint8_t* pixels, int width, int height) {
    size_t row_bytes = (size_t)width * 4;
    uint8_t row_tmp[4096]; // stack buffer for small rows
    uint8_t* heap_tmp = nullptr;
    uint8_t* tmp = row_tmp;
    if (row_bytes > sizeof(row_tmp)) {
        heap_tmp = new uint8_t[row_bytes];
        tmp = heap_tmp;
    }
    for (int y = 0; y < height / 2; y++) {
        uint8_t* top = pixels + y * row_bytes;
        uint8_t* bot = pixels + (height - 1 - y) * row_bytes;
        std::memcpy(tmp, top, row_bytes);
        std::memcpy(top, bot, row_bytes);
        std::memcpy(bot, tmp, row_bytes);
    }
    delete[] heap_tmp;
}
