#pragma once
#include <cstdint>
#include <cstring>

namespace BlendOps {

/// Alpha-composite src RGBA pixel over dst RGBA pixel (Porter-Duff "over").
inline void blend_over(uint8_t* dst, const uint8_t* src) {
    uint8_t sa = src[3];
    if (sa == 0) return;
    if (sa == 255) { std::memcpy(dst, src, 4); return; }
    float a = sa / 255.0f;
    float inv = 1.0f - a;
    float da = dst[3] / 255.0f;
    float oa = a + da * inv;
    if (oa > 0) {
        dst[0] = (uint8_t)((src[0] * a + dst[0] * da * inv) / oa);
        dst[1] = (uint8_t)((src[1] * a + dst[1] * da * inv) / oa);
        dst[2] = (uint8_t)((src[2] * a + dst[2] * da * inv) / oa);
        dst[3] = (uint8_t)(oa * 255);
    }
}

/// Composite a colored pixel with a given alpha over dst RGBA pixel.
inline void blend_color(uint8_t* dst, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha) {
    if (alpha == 0) return;
    if (alpha == 255) { dst[0] = r; dst[1] = g; dst[2] = b; dst[3] = 255; return; }
    float a = alpha / 255.0f;
    float inv = 1.0f - a;
    float da = dst[3] / 255.0f;
    float oa = a + da * inv;
    if (oa > 0) {
        dst[0] = (uint8_t)((r * a + dst[0] * da * inv) / oa);
        dst[1] = (uint8_t)((g * a + dst[1] * da * inv) / oa);
        dst[2] = (uint8_t)((b * a + dst[2] * da * inv) / oa);
        dst[3] = (uint8_t)(oa * 255);
    }
}

} // namespace BlendOps
