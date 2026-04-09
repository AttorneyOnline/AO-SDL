#include <gtest/gtest.h>

#include "utils/Crypto.h"

// PBKDF2-HMAC-SHA256 test vectors from RFC 7914 and akashi compatibility.

TEST(CryptoPBKDF2, RFC7914_Vector1) {
    // RFC 7914, Section 11: password="passwd", salt="salt", c=1, dkLen=64
    // We only use the first 32 bytes since our default output is 32.
    std::vector<uint8_t> pw = {'p', 'a', 's', 's', 'w', 'd'};
    std::vector<uint8_t> salt = {'s', 'a', 'l', 't'};
    auto dk = crypto::pbkdf2_sha256(pw, salt, 1, 32);

    std::ostringstream ss;
    for (auto b : dk)
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(b);

    EXPECT_EQ(ss.str(), "55ac046e56e3089fec1691c22544b605f94185216dde0465e68b9d57c20dacbc");
}

TEST(CryptoPBKDF2, SingleIteration) {
    // PBKDF2(password="password", salt="salt", c=1, dkLen=32)
    auto result = crypto::pbkdf2_sha256_hex("password", "73616c74", 1); // "salt" = 73616c74 hex
    EXPECT_EQ(result, "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b");
}

TEST(CryptoPBKDF2, TwoIterations) {
    // PBKDF2(password="password", salt="salt", c=2, dkLen=32)
    auto result = crypto::pbkdf2_sha256_hex("password", "73616c74", 2); // "salt" = 73616c74 hex
    EXPECT_EQ(result, "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43");
}

TEST(CryptoPBKDF2, HexConvenience) {
    // Verify the hex convenience function matches the raw function.
    std::string password = "testpassword";
    std::string salt_hex = "a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6";

    auto result = crypto::pbkdf2_sha256_hex(password, salt_hex, 1);

    // Manually compute via raw API
    std::vector<uint8_t> pw(password.begin(), password.end());
    std::vector<uint8_t> salt = {0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0xa7, 0xb8,
                                 0xc9, 0xd0, 0xe1, 0xf2, 0xa3, 0xb4, 0xc5, 0xd6};
    auto dk = crypto::pbkdf2_sha256(pw, salt, 1, 32);

    std::ostringstream ss;
    for (auto b : dk)
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(b);

    EXPECT_EQ(result, ss.str());
}

TEST(CryptoPBKDF2, AkashiCompatibility) {
    // Verify output matches akashi's CryptoHelper::pbkdf2() for a known input.
    // This was independently verified against Python's hashlib.pbkdf2_hmac.
    std::string password = "testpassword";
    std::string salt_hex = "a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6";

    // 100,000 iterations (akashi default) — this takes ~1s
    auto result = crypto::pbkdf2_sha256_hex(password, salt_hex, 100000);
    EXPECT_EQ(result, "04e240115a658b6419ffbcbd0d8267cebcb1425ccf98098afc7019a39cdd0b98");
}

// -- HMAC-SHA256 tests --------------------------------------------------------

TEST(CryptoHMAC, EmptyKeyAndMessage) {
    auto result = crypto::hmac_sha256_hex("", "");
    EXPECT_EQ(result, "b613679a0814d9ec772f95d778c35fc5ff1697c493715653c6c712144292c5ad");
}

TEST(CryptoHMAC, RFC4231_Vector2) {
    // RFC 4231 Test Case 2: key="Jefe", data="what do ya want for nothing?"
    auto result = crypto::hmac_sha256_hex("Jefe", "what do ya want for nothing?");
    EXPECT_EQ(result, "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
}

TEST(CryptoHMAC, AkashiLegacyCompat) {
    // Akashi legacy path: HMAC(key=salt_hex_string, data=password)
    // salt_hex is used as the key string directly (not decoded).
    auto result = crypto::hmac_sha256_hex("aabbccdd", "mypassword");
    EXPECT_EQ(result, "e6b48c1f615ad1f00cc2cffee594c2026cc2241c49d38d356905a25cef239ffc");
}

// -- Random bytes -------------------------------------------------------------

TEST(CryptoRandom, CorrectLength) {
    auto r16 = crypto::randbytes(16);
    EXPECT_EQ(r16.size(), 16u);

    auto r32 = crypto::randbytes(32);
    EXPECT_EQ(r32.size(), 32u);

    auto r0 = crypto::randbytes(0);
    EXPECT_EQ(r0.size(), 0u);
}

TEST(CryptoRandom, NotAllZeros) {
    // Probabilistic: 32 random bytes should not be all zeros.
    auto r = crypto::randbytes(32);
    bool all_zero = true;
    for (auto b : r) {
        if (b != 0) {
            all_zero = false;
            break;
        }
    }
    EXPECT_FALSE(all_zero);
}

TEST(CryptoRandom, HexLength) {
    auto hex = crypto::randbytes_hex(16);
    EXPECT_EQ(hex.size(), 32u); // 16 bytes = 32 hex chars
}
