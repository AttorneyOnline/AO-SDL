#include "utils/UTF8.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <string>

// ---------------------------------------------------------------------------
// decode
// ---------------------------------------------------------------------------

TEST(UTF8Decode, AsciiCharacter) {
    std::string s = "A";
    size_t pos = 0;
    EXPECT_EQ(UTF8::decode(s, pos), 0x41u);
    EXPECT_EQ(pos, 1u);
}

TEST(UTF8Decode, TwoByteCharacter) {
    // U+00F1 LATIN SMALL LETTER N WITH TILDE: 0xC3 0xB1
    std::string s = "\xC3\xB1";
    size_t pos = 0;
    EXPECT_EQ(UTF8::decode(s, pos), 0x00F1u);
    EXPECT_EQ(pos, 2u);
}

TEST(UTF8Decode, ThreeByteCharacter) {
    // U+20AC EURO SIGN: 0xE2 0x82 0xAC
    std::string s = "\xE2\x82\xAC";
    size_t pos = 0;
    EXPECT_EQ(UTF8::decode(s, pos), 0x20ACu);
    EXPECT_EQ(pos, 3u);
}

TEST(UTF8Decode, FourByteCharacter) {
    // U+1F600 GRINNING FACE: 0xF0 0x9F 0x98 0x80
    std::string s = "\xF0\x9F\x98\x80";
    size_t pos = 0;
    EXPECT_EQ(UTF8::decode(s, pos), 0x1F600u);
    EXPECT_EQ(pos, 4u);
}

TEST(UTF8Decode, EmptyStringReturnsZero) {
    std::string s;
    size_t pos = 0;
    EXPECT_EQ(UTF8::decode(s, pos), 0u);
}

TEST(UTF8Decode, InvalidByteReturnsReplacement) {
    // 0xFF is not a valid UTF-8 leading byte
    std::string s = "\xFF";
    size_t pos = 0;
    EXPECT_EQ(UTF8::decode(s, pos), 0xFFFDu);
    EXPECT_EQ(pos, 1u);
}

// ---------------------------------------------------------------------------
// length
// ---------------------------------------------------------------------------

TEST(UTF8Length, EmptyString) {
    EXPECT_EQ(UTF8::length(""), 0);
}

TEST(UTF8Length, AsciiString) {
    EXPECT_EQ(UTF8::length("hello"), 5);
}

TEST(UTF8Length, MixedAsciiAndMultibyte) {
    // "n" (1 byte) + U+00F1 (2 bytes) + "o" (1 byte) = 3 characters
    std::string s = "n\xC3\xB1o";
    EXPECT_EQ(UTF8::length(s), 3);
}

TEST(UTF8Length, EmojiCountsAsOne) {
    // Two grinning faces: each is 4 bytes, but 2 codepoints
    std::string s = "\xF0\x9F\x98\x80\xF0\x9F\x98\x80";
    EXPECT_EQ(UTF8::length(s), 2);
}

// ---------------------------------------------------------------------------
// byte_offset
// ---------------------------------------------------------------------------

TEST(UTF8ByteOffset, IndexZeroReturnsZero) {
    std::string s = "hello";
    EXPECT_EQ(UTF8::byte_offset(s, 0), 0u);
}

TEST(UTF8ByteOffset, AsciiStringMatchesCharIdx) {
    std::string s = "abcde";
    EXPECT_EQ(UTF8::byte_offset(s, 3), 3u);
}

TEST(UTF8ByteOffset, AfterTwoByteCharReturnsTwo) {
    // U+00F1 (2 bytes) followed by 'x'
    std::string s = "\xC3\xB1x";
    EXPECT_EQ(UTF8::byte_offset(s, 1), 2u);
}

TEST(UTF8ByteOffset, BeyondEndReturnsStringSize) {
    std::string s = "ab";
    EXPECT_EQ(UTF8::byte_offset(s, 100), s.size());
}
