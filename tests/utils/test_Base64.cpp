#include "utils/Base64.h"

#include <gtest/gtest.h>
#include <cstdint>
#include <string>
#include <vector>

// Helper: build a span from a string literal's bytes.
static std::span<const uint8_t> as_bytes(std::string_view s) {
    return {reinterpret_cast<const uint8_t*>(s.data()), s.size()};
}

// ---------------------------------------------------------------------------
// encode
// ---------------------------------------------------------------------------

TEST(Base64Encode, EmptyInput) {
    EXPECT_EQ(Base64::encode(std::span<const uint8_t>{}), "");
}

TEST(Base64Encode, SingleByte) {
    // 0x4D ('M') → "TQ=="
    std::vector<uint8_t> data = {0x4D};
    EXPECT_EQ(Base64::encode(data), "TQ==");
}

TEST(Base64Encode, TwoBytes) {
    // 0x4D 0x61 → "TWE="
    std::vector<uint8_t> data = {0x4D, 0x61};
    EXPECT_EQ(Base64::encode(data), "TWE=");
}

TEST(Base64Encode, ThreeByteAligned) {
    // "Man" → "TWFu"
    EXPECT_EQ(Base64::encode(as_bytes("Man")), "TWFu");
}

TEST(Base64Encode, KnownString) {
    // "hello" → "aGVsbG8="
    EXPECT_EQ(Base64::encode(as_bytes("hello")), "aGVsbG8=");
}

TEST(Base64Encode, AllZeroBytes) {
    std::vector<uint8_t> data(3, 0x00);
    EXPECT_EQ(Base64::encode(data), "AAAA");
}

TEST(Base64Encode, LongerString) {
    // "Many hands make light work." → known RFC 4648 value
    EXPECT_EQ(Base64::encode(as_bytes("Many hands make light work.")),
              "TWFueSBoYW5kcyBtYWtlIGxpZ2h0IHdvcmsu");
}

TEST(Base64Encode, OutputLengthIsMultipleOfFour) {
    for (size_t len = 0; len <= 16; ++len) {
        std::vector<uint8_t> data(len, 0xAB);
        EXPECT_EQ(Base64::encode(data).size() % 4, 0u) << "length=" << len;
    }
}

// ---------------------------------------------------------------------------
// decode — API note
// ---------------------------------------------------------------------------
// Base64::decode returns std::span<const uint8_t> pointing into a local
// std::vector that is destroyed on return (dangling span / UB). Decoding
// cannot be safely tested until the return type is changed to std::vector or
// the caller supplies an output buffer.  The tests below only exercise the
// contract for *invalid* input, which throws before any result is returned.

TEST(Base64Decode, ThrowsOnNonMultipleOfFourLength) {
    std::string bad = "abc";   // length 3, not a multiple of 4
    EXPECT_THROW(Base64::decode(bad), std::invalid_argument);
}

TEST(Base64Decode, ThrowsOnLengthFive) {
    std::string bad = "aGVsb";
    EXPECT_THROW(Base64::decode(bad), std::invalid_argument);
}

TEST(Base64Decode, EmptyStringDoesNotThrow) {
    std::string empty;
    EXPECT_NO_THROW(Base64::decode(empty));
}
