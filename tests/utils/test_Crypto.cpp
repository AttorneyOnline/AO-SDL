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

// Padding boundary tests: exercise every edge case in the final-block logic.
// Block size is 64 bytes for both SHA-1 and SHA-256.
//   55 bytes: last size that fits padding + 8-byte length in one block
//   56 bytes: first size requiring two blocks for padding (covered by NIST_TwoBlock)
//   63 bytes: last sub-block size
//   64 bytes: exactly one full block (covered by ExactBlockBoundary)
//   65 bytes: one full block + 1 byte remainder
//  119 bytes: fills second block to padding boundary
//  120 bytes: two blocks, padding needs a third block
//  128 bytes: exactly two full blocks

TEST(CryptoSHA1, Padding_55bytes) {
    EXPECT_EQ(crypto::sha1(std::string(55, 'a')), "c1c8bbdc22796e28c0e15163d20899b65621d65a");
}

TEST(CryptoSHA1, Padding_63bytes) {
    EXPECT_EQ(crypto::sha1(std::string(63, 'a')), "03f09f5b158a7a8cdad920bddc29b81c18a551f5");
}

TEST(CryptoSHA1, Padding_64bytes) {
    EXPECT_EQ(crypto::sha1(std::string(64, 'a')), "0098ba824b5c16427bd7a1122a5a442a25ec644d");
}

TEST(CryptoSHA1, Padding_65bytes) {
    EXPECT_EQ(crypto::sha1(std::string(65, 'a')), "11655326c708d70319be2610e8a57d9a5b959d3b");
}

TEST(CryptoSHA1, Padding_119bytes) {
    EXPECT_EQ(crypto::sha1(std::string(119, 'a')), "ee971065aaa017e0632a8ca6c77bb3bf8b1dfc56");
}

TEST(CryptoSHA1, Padding_120bytes) {
    EXPECT_EQ(crypto::sha1(std::string(120, 'a')), "f34c1488385346a55709ba056ddd08280dd4c6d6");
}

TEST(CryptoSHA1, Padding_128bytes) {
    EXPECT_EQ(crypto::sha1(std::string(128, 'a')), "ad5b3fdbcb526778c2839d2f151ea753995e26a0");
}

TEST(CryptoSHA256, Padding_55bytes) {
    EXPECT_EQ(crypto::sha256(std::string(55, 'a')), "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318");
}

TEST(CryptoSHA256, Padding_63bytes) {
    EXPECT_EQ(crypto::sha256(std::string(63, 'a')), "7d3e74a05d7db15bce4ad9ec0658ea98e3f06eeecf16b4c6fff2da457ddc2f34");
}

TEST(CryptoSHA256, Padding_65bytes) {
    EXPECT_EQ(crypto::sha256(std::string(65, 'a')), "635361c48bb9eab14198e76ea8ab7f1a41685d6ad62aa9146d301d4f17eb0ae0");
}

TEST(CryptoSHA256, Padding_119bytes) {
    EXPECT_EQ(crypto::sha256(std::string(119, 'a')),
              "31eba51c313a5c08226adf18d4a359cfdfd8d2e816b13f4af952f7ea6584dcfb");
}

TEST(CryptoSHA256, Padding_120bytes) {
    EXPECT_EQ(crypto::sha256(std::string(120, 'a')),
              "2f3d335432c70b580af0e8e1b3674a7c020d683aa5f73aaaedfdc55af904c21c");
}

TEST(CryptoSHA256, Padding_128bytes) {
    EXPECT_EQ(crypto::sha256(std::string(128, 'a')),
              "6836cf13bac400e9105071cd6af47084dfacad4e5e302c94bfed24e013afb73e");
}

// -- Raw output tests --------------------------------------------------------

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
