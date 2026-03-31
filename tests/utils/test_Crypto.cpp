#include <gtest/gtest.h>

#include "utils/Crypto.h"

// SHA-1 test vectors from NIST FIPS 180-4 and RFC 6455.

TEST(CryptoSHA1, Empty) {
    EXPECT_EQ(crypto::sha1(""), "da39a3ee5e6b4b0d3255bfef95601890afd80709");
}

TEST(CryptoSHA1, ABC) {
    EXPECT_EQ(crypto::sha1("abc"), "a9993e364706816aba3e25717850c26c9cd0d89d");
}

TEST(CryptoSHA1, NIST_TwoBlock) {
    // "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" (56 bytes, triggers two-block padding)
    EXPECT_EQ(crypto::sha1("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
              "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
}

TEST(CryptoSHA1, RFC6455_AcceptKey) {
    // WebSocket handshake: SHA-1 of client key + magic GUID
    std::string input = "dGhlIHNhbXBsZSBub25jZQ==258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    EXPECT_EQ(crypto::sha1(input), "b37a4f2cc0624f1690f64606cf385945b2bec4ea");
}

TEST(CryptoSHA1, RawLength) {
    auto raw = crypto::sha1_raw("abc");
    EXPECT_EQ(raw.size(), 20u);
}

TEST(CryptoSHA1, RawMatchesHex) {
    auto hex = crypto::sha1("test");
    auto raw = crypto::sha1_raw("test");

    std::ostringstream ss;
    for (uint8_t c : raw)
        ss << std::hex << std::setfill('0') << std::setw(2) << (int)c;
    EXPECT_EQ(ss.str(), hex);
}

TEST(CryptoSHA256, RawLength) {
    auto raw = crypto::sha256_raw("abc");
    EXPECT_EQ(raw.size(), 32u);
}

TEST(CryptoSHA256, RawMatchesHex) {
    auto hex = crypto::sha256("test");
    auto raw = crypto::sha256_raw("test");

    std::ostringstream ss;
    for (uint8_t c : raw)
        ss << std::hex << std::setfill('0') << std::setw(2) << (int)c;
    EXPECT_EQ(ss.str(), hex);
}

// SHA-256 test vectors from NIST FIPS 180-4.

TEST(CryptoSHA256, Empty) {
    EXPECT_EQ(crypto::sha256(""), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(CryptoSHA256, ABC) {
    EXPECT_EQ(crypto::sha256("abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(CryptoSHA256, NIST_TwoBlock) {
    // "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" (56 bytes)
    EXPECT_EQ(crypto::sha256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
              "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}

TEST(CryptoSHA256, NIST_LongTwoBlock) {
    // "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"
    // (112 bytes)
    EXPECT_EQ(crypto::sha256("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"
                             "ijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"),
              "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1");
}

TEST(CryptoSHA256, OutputLength) {
    // SHA-256 hex output should always be 64 chars
    EXPECT_EQ(crypto::sha256("").size(), 64u);
    EXPECT_EQ(crypto::sha256("hello world").size(), 64u);
}

TEST(CryptoSHA256, SingleChar) {
    EXPECT_EQ(crypto::sha256("a"), "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb");
}

TEST(CryptoSHA256, ExactBlockBoundary) {
    // 64 bytes = exactly one SHA-256 block
    std::string s64(64, 'a');
    EXPECT_EQ(crypto::sha256(s64), "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb");
}
