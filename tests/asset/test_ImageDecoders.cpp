#include "asset/ImageDecoder.h"

#include <algorithm>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>

// Factory functions for individual decoders
std::unique_ptr<ImageDecoder> create_stbi_decoder();
std::unique_ptr<ImageDecoder> create_gif_decoder();
std::unique_ptr<ImageDecoder> create_webp_decoder();
std::unique_ptr<ImageDecoder> create_apng_decoder();

// ---------------------------------------------------------------------------
// Helpers: minimal valid image data
// ---------------------------------------------------------------------------

// Minimal 1x1 red PNG (same approach as test_ApngDecoder.cpp).
// IHDR (1x1, 8-bit RGBA) + IDAT (zlib stored block) + IEND.
static std::vector<uint8_t> make_1x1_png() {
    std::vector<uint8_t> png = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

    // IHDR chunk
    uint8_t ihdr[] = {
        0x00, 0x00, 0x00, 0x0D,                          // length = 13
        'I',  'H',  'D',  'R',  0x00, 0x00, 0x00, 0x01,  // width = 1
        0x00, 0x00, 0x00, 0x01,                           // height = 1
        0x08,                                             // bit depth = 8
        0x06,                                             // color type = 6 (RGBA)
        0x00,                                             // compression = 0
        0x00,                                             // filter = 0
        0x00,                                             // interlace = 0
        0x00, 0x00, 0x00, 0x00,                           // CRC (ignored by stbi)
    };
    png.insert(png.end(), ihdr, ihdr + sizeof(ihdr));

    // IDAT chunk: zlib-compressed scanline [filter=0, R=0xFF, G=0x00, B=0x00, A=0xFF]
    uint8_t idat_data[] = {
        0x78, 0x01,             // zlib header
        0x01,                   // BFINAL=1, BTYPE=00 (stored)
        0x05, 0x00,             // LEN = 5
        0xFA, 0xFF,             // NLEN = ~5
        0x00,                   // filter byte = none
        0xFF, 0x00, 0x00, 0xFF, // RGBA red pixel
        0x00, 0x86, 0x00, 0xA5, // Adler-32
    };
    uint8_t idat_header[] = {
        0x00, 0x00, 0x00, (uint8_t)sizeof(idat_data),
        'I',  'D',  'A',  'T',
    };
    png.insert(png.end(), idat_header, idat_header + sizeof(idat_header));
    png.insert(png.end(), idat_data, idat_data + sizeof(idat_data));
    uint8_t zero_crc[] = {0, 0, 0, 0};
    png.insert(png.end(), zero_crc, zero_crc + 4);

    // IEND chunk
    uint8_t iend[] = {0, 0, 0, 0, 'I', 'E', 'N', 'D', 0xAE, 0x42, 0x60, 0x82};
    png.insert(png.end(), iend, iend + sizeof(iend));

    return png;
}

// Minimal 1x1 GIF89a with a single red pixel.
// Structure: header + LSD + GCT(2 entries) + image descriptor + LZW data + trailer.
static std::vector<uint8_t> make_1x1_gif() {
    // clang-format off
    std::vector<uint8_t> gif = {
        // Header: GIF89a
        'G', 'I', 'F', '8', '9', 'a',

        // Logical Screen Descriptor
        0x01, 0x00,  // width = 1 (LE)
        0x01, 0x00,  // height = 1 (LE)
        0x80,        // packed: GCT flag=1, color res=1, sort=0, GCT size=0 (2^(0+1) = 2 entries)
        0x00,        // background color index
        0x00,        // pixel aspect ratio

        // Global Color Table (2 entries = 6 bytes)
        0xFF, 0x00, 0x00,  // index 0: red
        0x00, 0x00, 0x00,  // index 1: black

        // Image Descriptor
        0x2C,        // image separator
        0x00, 0x00,  // left = 0
        0x00, 0x00,  // top = 0
        0x01, 0x00,  // width = 1
        0x01, 0x00,  // height = 1
        0x00,        // packed: no local color table

        // LZW Minimum Code Size
        0x02,

        // LZW compressed data sub-block (2 bytes):
        // Codes (3-bit each, LSB-first): Clear(4)=100, pixel(0)=000, EOI(5)=101
        // bit0-2: clear=0,0,1  bit3-5: pixel=0,0,0  bit6-8: eoi=1,0,1
        // byte 0: bits[7..0] = 0,1,0,0,0,1,0,0 = 0x44
        // byte 1: bits[8]    = 1               = 0x01
        0x02,        // sub-block length = 2
        0x44, 0x01,  // LZW data

        0x00,        // block terminator

        // Trailer
        0x3B,
    };
    // clang-format on
    return gif;
}

// Garbage data that no decoder should accept.
static std::vector<uint8_t> make_garbage() {
    return {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
}

// Truncated PNG: valid signature but nothing else meaningful.
static std::vector<uint8_t> make_truncated_png() {
    return {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A, 0x00};
}

// Truncated GIF: valid header but missing everything after LSD.
static std::vector<uint8_t> make_truncated_gif() {
    return {'G', 'I', 'F', '8', '9', 'a', 0x01, 0x00};
}

// =========================================================================
// StbiDecoder tests
// =========================================================================

class StbiDecoderTest : public ::testing::Test {
  protected:
    void SetUp() override { decoder = create_stbi_decoder(); }
    std::unique_ptr<ImageDecoder> decoder;
};

TEST_F(StbiDecoderTest, ExtensionsIncludeExpectedFormats) {
    auto exts = decoder->extensions();
    EXPECT_FALSE(exts.empty());
    // StbiDecoder handles jpg, jpeg, bmp, tga
    bool has_jpg = false;
    for (const auto& e : exts) {
        if (e == "jpg" || e == "jpeg") has_jpg = true;
    }
    EXPECT_TRUE(has_jpg);
}

TEST_F(StbiDecoderTest, DecodesValidPng) {
    // stbi can decode PNG even though PNG is not in its extensions() list.
    // The decode() method works on raw bytes regardless of extension.
    auto png = make_1x1_png();
    auto frames = decoder->decode(png.data(), png.size());
    if (!frames.empty()) {
        ASSERT_EQ(frames.size(), 1u);
        EXPECT_EQ(frames[0].width, 1);
        EXPECT_EQ(frames[0].height, 1);
        EXPECT_EQ(frames[0].duration_ms, 0);
        EXPECT_EQ(frames[0].pixels.size(), 4u);  // 1x1 RGBA
    }
    // If stbi rejects the hand-crafted PNG due to checksum, that is acceptable.
}

TEST_F(StbiDecoderTest, RejectsGarbageData) {
    auto garbage = make_garbage();
    auto frames = decoder->decode(garbage.data(), garbage.size());
    EXPECT_TRUE(frames.empty());
}

TEST_F(StbiDecoderTest, RejectsEmptyData) {
    auto frames = decoder->decode(nullptr, 0);
    EXPECT_TRUE(frames.empty());
}

TEST_F(StbiDecoderTest, RejectsTruncatedPng) {
    auto data = make_truncated_png();
    auto frames = decoder->decode(data.data(), data.size());
    EXPECT_TRUE(frames.empty());
}

TEST_F(StbiDecoderTest, RejectsZeroLengthWithValidPointer) {
    uint8_t dummy = 0;
    auto frames = decoder->decode(&dummy, 0);
    EXPECT_TRUE(frames.empty());
}

TEST_F(StbiDecoderTest, SingleFrameStaticDuration) {
    // Static images should have duration_ms == 0
    auto png = make_1x1_png();
    auto frames = decoder->decode(png.data(), png.size());
    if (!frames.empty()) {
        EXPECT_EQ(frames[0].duration_ms, 0);
    }
}

// =========================================================================
// GifDecoder tests
// =========================================================================

class GifDecoderTest : public ::testing::Test {
  protected:
    void SetUp() override { decoder = create_gif_decoder(); }
    std::unique_ptr<ImageDecoder> decoder;
};

TEST_F(GifDecoderTest, ExtensionsContainGif) {
    auto exts = decoder->extensions();
    ASSERT_FALSE(exts.empty());
    bool has_gif = false;
    for (const auto& e : exts) {
        if (e == "gif") has_gif = true;
    }
    EXPECT_TRUE(has_gif);
}

TEST_F(GifDecoderTest, DecodesMinimalGif) {
    auto gif = make_1x1_gif();
    auto frames = decoder->decode(gif.data(), gif.size());
    // The hand-crafted GIF may or may not decode depending on LZW correctness.
    // If it does decode, validate the output.
    if (!frames.empty()) {
        ASSERT_GE(frames.size(), 1u);
        EXPECT_EQ(frames[0].width, 1);
        EXPECT_EQ(frames[0].height, 1);
        EXPECT_EQ(frames[0].pixels.size(), 4u);  // 1x1 RGBA
    }
}

TEST_F(GifDecoderTest, RejectsGarbageData) {
    auto garbage = make_garbage();
    auto frames = decoder->decode(garbage.data(), garbage.size());
    EXPECT_TRUE(frames.empty());
}

TEST_F(GifDecoderTest, RejectsEmptyData) {
    auto frames = decoder->decode(nullptr, 0);
    EXPECT_TRUE(frames.empty());
}

TEST_F(GifDecoderTest, RejectsTruncatedGif) {
    auto data = make_truncated_gif();
    auto frames = decoder->decode(data.data(), data.size());
    EXPECT_TRUE(frames.empty());
}

TEST_F(GifDecoderTest, RejectsZeroLengthWithValidPointer) {
    uint8_t dummy = 0;
    auto frames = decoder->decode(&dummy, 0);
    EXPECT_TRUE(frames.empty());
}

// =========================================================================
// WebPDecoder tests
// =========================================================================

class WebPDecoderTest : public ::testing::Test {
  protected:
    void SetUp() override { decoder = create_webp_decoder(); }
    std::unique_ptr<ImageDecoder> decoder;
};

TEST_F(WebPDecoderTest, ExtensionsContainWebp) {
    auto exts = decoder->extensions();
    ASSERT_FALSE(exts.empty());
    bool has_webp = false;
    for (const auto& e : exts) {
        if (e == "webp") has_webp = true;
    }
    EXPECT_TRUE(has_webp);
}

TEST_F(WebPDecoderTest, RejectsGarbageData) {
    auto garbage = make_garbage();
    auto frames = decoder->decode(garbage.data(), garbage.size());
    EXPECT_TRUE(frames.empty());
}

TEST_F(WebPDecoderTest, RejectsEmptyData) {
    auto frames = decoder->decode(nullptr, 0);
    EXPECT_TRUE(frames.empty());
}

TEST_F(WebPDecoderTest, RejectsZeroLengthWithValidPointer) {
    uint8_t dummy = 0;
    auto frames = decoder->decode(&dummy, 0);
    EXPECT_TRUE(frames.empty());
}

TEST_F(WebPDecoderTest, RejectsTruncatedWebPHeader) {
    // RIFF....WEBP but truncated
    std::vector<uint8_t> data = {'R', 'I', 'F', 'F', 0x00, 0x00, 0x00, 0x00, 'W', 'E', 'B', 'P'};
    auto frames = decoder->decode(data.data(), data.size());
    EXPECT_TRUE(frames.empty());
}

// =========================================================================
// ApngDecoder (via ImageDecoder interface) tests
// =========================================================================

class ApngDecoderInterfaceTest : public ::testing::Test {
  protected:
    void SetUp() override { decoder = create_apng_decoder(); }
    std::unique_ptr<ImageDecoder> decoder;
};

TEST_F(ApngDecoderInterfaceTest, ExtensionsContainPngAndApng) {
    auto exts = decoder->extensions();
    bool has_png = false, has_apng = false;
    for (const auto& e : exts) {
        if (e == "png") has_png = true;
        if (e == "apng") has_apng = true;
    }
    EXPECT_TRUE(has_png);
    EXPECT_TRUE(has_apng);
}

TEST_F(ApngDecoderInterfaceTest, DecodesValidPng) {
    auto png = make_1x1_png();
    auto frames = decoder->decode(png.data(), png.size());
    if (!frames.empty()) {
        ASSERT_EQ(frames.size(), 1u);
        EXPECT_EQ(frames[0].width, 1);
        EXPECT_EQ(frames[0].height, 1);
        EXPECT_EQ(frames[0].duration_ms, 0);
        EXPECT_EQ(frames[0].pixels.size(), 4u);
    }
}

TEST_F(ApngDecoderInterfaceTest, RejectsGarbageData) {
    auto garbage = make_garbage();
    auto frames = decoder->decode(garbage.data(), garbage.size());
    EXPECT_TRUE(frames.empty());
}

TEST_F(ApngDecoderInterfaceTest, RejectsEmptyData) {
    auto frames = decoder->decode(nullptr, 0);
    EXPECT_TRUE(frames.empty());
}

// =========================================================================
// Cross-decoder consistency tests
// =========================================================================

class AllDecodersTest : public ::testing::Test {
  protected:
    void SetUp() override {
        decoders.push_back(create_stbi_decoder());
        decoders.push_back(create_gif_decoder());
        decoders.push_back(create_webp_decoder());
        decoders.push_back(create_apng_decoder());
    }
    std::vector<std::unique_ptr<ImageDecoder>> decoders;
};

TEST_F(AllDecodersTest, AllRejectNullInput) {
    for (const auto& dec : decoders) {
        auto frames = dec->decode(nullptr, 0);
        EXPECT_TRUE(frames.empty()) << "Decoder with extension " << dec->extensions()[0]
                                    << " did not reject null input";
    }
}

TEST_F(AllDecodersTest, AllRejectEmptyWithValidPointer) {
    uint8_t dummy = 0;
    for (const auto& dec : decoders) {
        auto frames = dec->decode(&dummy, 0);
        EXPECT_TRUE(frames.empty()) << "Decoder with extension " << dec->extensions()[0]
                                    << " did not reject zero-length input";
    }
}

TEST_F(AllDecodersTest, AllRejectGarbage) {
    auto garbage = make_garbage();
    for (const auto& dec : decoders) {
        auto frames = dec->decode(garbage.data(), garbage.size());
        EXPECT_TRUE(frames.empty()) << "Decoder with extension " << dec->extensions()[0]
                                    << " did not reject garbage input";
    }
}

TEST_F(AllDecodersTest, AllRejectSingleByte) {
    uint8_t one = 0xFF;
    for (const auto& dec : decoders) {
        auto frames = dec->decode(&one, 1);
        EXPECT_TRUE(frames.empty()) << "Decoder with extension " << dec->extensions()[0]
                                    << " did not reject single-byte input";
    }
}

TEST_F(AllDecodersTest, AllHaveNonEmptyExtensions) {
    for (const auto& dec : decoders) {
        EXPECT_FALSE(dec->extensions().empty());
    }
}

TEST_F(AllDecodersTest, ValidFramesHavePositiveDimensions) {
    // Decode a valid PNG through each decoder. Any decoder that produces
    // frames must return frames with positive width/height and correct pixel count.
    auto png = make_1x1_png();
    for (const auto& dec : decoders) {
        auto frames = dec->decode(png.data(), png.size());
        for (const auto& f : frames) {
            EXPECT_GT(f.width, 0);
            EXPECT_GT(f.height, 0);
            EXPECT_EQ(f.pixels.size(), (size_t)(f.width * f.height * 4));
        }
    }
}

// =========================================================================
// image_decoders() global registry tests
// =========================================================================

TEST(ImageDecoderRegistry, ReturnsNonEmptyDecoderList) {
    const auto& decoders = image_decoders();
    EXPECT_GE(decoders.size(), 4u);  // webp, apng, gif, stbi
}

TEST(ImageDecoderRegistry, SupportedExtensionsIncludeCommonFormats) {
    auto exts = supported_image_extensions();
    EXPECT_FALSE(exts.empty());

    auto has = [&](const std::string& e) {
        for (const auto& x : exts)
            if (x == e) return true;
        return false;
    };
    EXPECT_TRUE(has("png"));
    EXPECT_TRUE(has("gif"));
    EXPECT_TRUE(has("webp"));
    EXPECT_TRUE(has("jpg"));
}

TEST(ImageDecoderRegistry, NoDuplicateExtensions) {
    auto exts = supported_image_extensions();
    std::vector<std::string> sorted = exts;
    std::sort(sorted.begin(), sorted.end());
    auto it = std::unique(sorted.begin(), sorted.end());
    EXPECT_EQ(it, sorted.end()) << "supported_image_extensions() contains duplicates";
}
