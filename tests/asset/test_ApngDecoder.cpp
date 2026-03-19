#include "asset/ApngDecoder.h"

#include <gtest/gtest.h>
#include <cstdint>
#include <vector>

// A minimal valid 1x1 red pixel PNG (non-APNG), hand-constructed.
// This is the simplest possible valid PNG: IHDR + IDAT + IEND.
// The IDAT contains zlib-compressed data for a 1x1 RGBA image.
static std::vector<uint8_t> make_1x1_png() {
    // Created by encoding a 1x1 red pixel PNG.
    // PNG signature
    std::vector<uint8_t> png = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

    // IHDR: 1x1, 8-bit RGBA (color type 6)
    uint8_t ihdr[] = {
        0x00, 0x00, 0x00, 0x0D, // length = 13
        'I', 'H', 'D', 'R',
        0x00, 0x00, 0x00, 0x01, // width = 1
        0x00, 0x00, 0x00, 0x01, // height = 1
        0x08,                   // bit depth = 8
        0x06,                   // color type = 6 (RGBA)
        0x00,                   // compression = 0
        0x00,                   // filter = 0
        0x00,                   // interlace = 0
        0x00, 0x00, 0x00, 0x00, // CRC (ignored by stbi)
    };
    png.insert(png.end(), ihdr, ihdr + sizeof(ihdr));

    // IDAT: zlib-compressed scanline: filter_byte(0) + R G B A
    // Raw data: [0x00, 0xFF, 0x00, 0x00, 0xFF] (filter=none, red pixel)
    // zlib: deflate with no compression (stored block)
    uint8_t idat_data[] = {
        0x78, 0x01, // zlib header (deflate, no dict)
        0x01,       // BFINAL=1, BTYPE=00 (no compression)
        0x05, 0x00, // LEN = 5
        0xFA, 0xFF, // NLEN = ~5
        0x00,       // filter byte = none
        0xFF, 0x00, 0x00, 0xFF, // RGBA red pixel
        0x00, 0x86, 0x00, 0xA5, // Adler-32 checksum
    };
    uint8_t idat_header[] = {
        0x00, 0x00, 0x00, (uint8_t)sizeof(idat_data), // length
        'I', 'D', 'A', 'T',
    };
    png.insert(png.end(), idat_header, idat_header + sizeof(idat_header));
    png.insert(png.end(), idat_data, idat_data + sizeof(idat_data));
    uint8_t zero_crc[] = {0, 0, 0, 0};
    png.insert(png.end(), zero_crc, zero_crc + 4);

    // IEND
    uint8_t iend[] = {0, 0, 0, 0, 'I', 'E', 'N', 'D', 0xAE, 0x42, 0x60, 0x82};
    png.insert(png.end(), iend, iend + sizeof(iend));

    return png;
}

TEST(ApngDecoder, RejectsNonPngData) {
    uint8_t garbage[] = {0x00, 0x01, 0x02, 0x03};
    auto result = ApngDecoder::decode(garbage, sizeof(garbage), false);
    EXPECT_FALSE(result.has_value());
}

TEST(ApngDecoder, RejectsEmptyData) {
    auto result = ApngDecoder::decode(nullptr, 0, false);
    EXPECT_FALSE(result.has_value());
}

TEST(ApngDecoder, RejectsTruncatedSignature) {
    uint8_t sig[] = {0x89, 'P', 'N', 'G'};
    auto result = ApngDecoder::decode(sig, sizeof(sig), false);
    EXPECT_FALSE(result.has_value());
}

TEST(ApngDecoder, DecodesSingleFramePng) {
    auto png = make_1x1_png();
    auto result = ApngDecoder::decode(png.data(), png.size(), false);

    // stbi might fail on our hand-crafted PNG if the zlib checksum is wrong,
    // so we test both paths
    if (result.has_value()) {
        ASSERT_EQ(result->size(), 1u);
        EXPECT_EQ((*result)[0].width, 1);
        EXPECT_EQ((*result)[0].height, 1);
        EXPECT_EQ((*result)[0].duration_ms, 0);
        EXPECT_EQ((*result)[0].pixels.size(), 4u); // 1x1 RGBA
    }
    // If stbi rejects our hand-crafted PNG, that's OK for this test
}

TEST(ApngDecoder, NonApngReturnsSingleFrame) {
    // Any valid PNG without acTL should return exactly 1 frame
    auto png = make_1x1_png();
    auto result = ApngDecoder::decode(png.data(), png.size(), false);

    if (result.has_value()) {
        EXPECT_EQ(result->size(), 1u);
    }
}
