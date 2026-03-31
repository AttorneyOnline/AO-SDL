/**
 * @file Crypto.h
 * @brief Header-only SHA-1 and SHA-256 implementations.
 *
 * SHA-1 based on public domain code by Steve Reid, adapted by
 * Volker Diels-Grabsch and Zlatko Michailov.
 *
 * SHA-256 follows the same structure, implementing FIPS 180-4.
 */
#pragma once

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace crypto {

// -- SHA-1 -------------------------------------------------------------------

namespace detail::sha1 {

static constexpr size_t BLOCK_INTS = 16;
static constexpr size_t BLOCK_BYTES = BLOCK_INTS * 4;

inline uint32_t rol(uint32_t value, size_t bits) {
    return (value << bits) | (value >> (32 - bits));
}

inline uint32_t blk(const uint32_t block[BLOCK_INTS], size_t i) {
    return rol(block[(i + 13) & 15] ^ block[(i + 8) & 15] ^ block[(i + 2) & 15] ^ block[i], 1);
}

inline void R0(const uint32_t block[BLOCK_INTS], uint32_t v, uint32_t& w, uint32_t x, uint32_t y, uint32_t& z,
               size_t i) {
    z += ((w & (x ^ y)) ^ y) + block[i] + 0x5a827999 + rol(v, 5);
    w = rol(w, 30);
}

inline void R1(uint32_t block[BLOCK_INTS], uint32_t v, uint32_t& w, uint32_t x, uint32_t y, uint32_t& z, size_t i) {
    block[i] = blk(block, i);
    z += ((w & (x ^ y)) ^ y) + block[i] + 0x5a827999 + rol(v, 5);
    w = rol(w, 30);
}

inline void R2(uint32_t block[BLOCK_INTS], uint32_t v, uint32_t& w, uint32_t x, uint32_t y, uint32_t& z, size_t i) {
    block[i] = blk(block, i);
    z += (w ^ x ^ y) + block[i] + 0x6ed9eba1 + rol(v, 5);
    w = rol(w, 30);
}

inline void R3(uint32_t block[BLOCK_INTS], uint32_t v, uint32_t& w, uint32_t x, uint32_t y, uint32_t& z, size_t i) {
    block[i] = blk(block, i);
    z += (((w | x) & y) | (w & x)) + block[i] + 0x8f1bbcdc + rol(v, 5);
    w = rol(w, 30);
}

inline void R4(uint32_t block[BLOCK_INTS], uint32_t v, uint32_t& w, uint32_t x, uint32_t y, uint32_t& z, size_t i) {
    block[i] = blk(block, i);
    z += (w ^ x ^ y) + block[i] + 0xca62c1d6 + rol(v, 5);
    w = rol(w, 30);
}

inline void buf_to_block(const std::string& buf, uint32_t block[BLOCK_INTS]) {
    for (size_t i = 0; i < BLOCK_INTS; i++) {
        block[i] = (uint32_t)(uint8_t)buf[4 * i + 3] | (uint32_t)(uint8_t)buf[4 * i + 2] << 8 |
                   (uint32_t)(uint8_t)buf[4 * i + 1] << 16 | (uint32_t)(uint8_t)buf[4 * i + 0] << 24;
    }
}

// Copied verbatim from the original sha1.hpp (public domain).
// 4 rounds of 20 operations each, loop unrolled.
inline void transform(uint32_t digest[5], uint32_t block[BLOCK_INTS]) {
    uint32_t a = digest[0], b = digest[1], c = digest[2], d = digest[3], e = digest[4];

    /* Round 0 */
    R0(block, a, b, c, d, e, 0);
    R0(block, e, a, b, c, d, 1);
    R0(block, d, e, a, b, c, 2);
    R0(block, c, d, e, a, b, 3);
    R0(block, b, c, d, e, a, 4);
    R0(block, a, b, c, d, e, 5);
    R0(block, e, a, b, c, d, 6);
    R0(block, d, e, a, b, c, 7);
    R0(block, c, d, e, a, b, 8);
    R0(block, b, c, d, e, a, 9);
    R0(block, a, b, c, d, e, 10);
    R0(block, e, a, b, c, d, 11);
    R0(block, d, e, a, b, c, 12);
    R0(block, c, d, e, a, b, 13);
    R0(block, b, c, d, e, a, 14);
    R0(block, a, b, c, d, e, 15);
    R1(block, e, a, b, c, d, 0);
    R1(block, d, e, a, b, c, 1);
    R1(block, c, d, e, a, b, 2);
    R1(block, b, c, d, e, a, 3);
    /* Round 1 */
    R2(block, a, b, c, d, e, 4);
    R2(block, e, a, b, c, d, 5);
    R2(block, d, e, a, b, c, 6);
    R2(block, c, d, e, a, b, 7);
    R2(block, b, c, d, e, a, 8);
    R2(block, a, b, c, d, e, 9);
    R2(block, e, a, b, c, d, 10);
    R2(block, d, e, a, b, c, 11);
    R2(block, c, d, e, a, b, 12);
    R2(block, b, c, d, e, a, 13);
    R2(block, a, b, c, d, e, 14);
    R2(block, e, a, b, c, d, 15);
    R2(block, d, e, a, b, c, 0);
    R2(block, c, d, e, a, b, 1);
    R2(block, b, c, d, e, a, 2);
    R2(block, a, b, c, d, e, 3);
    R2(block, e, a, b, c, d, 4);
    R2(block, d, e, a, b, c, 5);
    R2(block, c, d, e, a, b, 6);
    R2(block, b, c, d, e, a, 7);
    /* Round 2 */
    R3(block, a, b, c, d, e, 8);
    R3(block, e, a, b, c, d, 9);
    R3(block, d, e, a, b, c, 10);
    R3(block, c, d, e, a, b, 11);
    R3(block, b, c, d, e, a, 12);
    R3(block, a, b, c, d, e, 13);
    R3(block, e, a, b, c, d, 14);
    R3(block, d, e, a, b, c, 15);
    R3(block, c, d, e, a, b, 0);
    R3(block, b, c, d, e, a, 1);
    R3(block, a, b, c, d, e, 2);
    R3(block, e, a, b, c, d, 3);
    R3(block, d, e, a, b, c, 4);
    R3(block, c, d, e, a, b, 5);
    R3(block, b, c, d, e, a, 6);
    R3(block, a, b, c, d, e, 7);
    R3(block, e, a, b, c, d, 8);
    R3(block, d, e, a, b, c, 9);
    R3(block, c, d, e, a, b, 10);
    R3(block, b, c, d, e, a, 11);
    /* Round 3 */
    R4(block, a, b, c, d, e, 12);
    R4(block, e, a, b, c, d, 13);
    R4(block, d, e, a, b, c, 14);
    R4(block, c, d, e, a, b, 15);
    R4(block, b, c, d, e, a, 0);
    R4(block, a, b, c, d, e, 1);
    R4(block, e, a, b, c, d, 2);
    R4(block, d, e, a, b, c, 3);
    R4(block, c, d, e, a, b, 4);
    R4(block, b, c, d, e, a, 5);
    R4(block, a, b, c, d, e, 6);
    R4(block, e, a, b, c, d, 7);
    R4(block, d, e, a, b, c, 8);
    R4(block, c, d, e, a, b, 9);
    R4(block, b, c, d, e, a, 10);
    R4(block, a, b, c, d, e, 11);
    R4(block, e, a, b, c, d, 12);
    R4(block, d, e, a, b, c, 13);
    R4(block, c, d, e, a, b, 14);
    R4(block, b, c, d, e, a, 15);

    digest[0] += a;
    digest[1] += b;
    digest[2] += c;
    digest[3] += d;
    digest[4] += e;
}

inline void compute(uint32_t digest[5], const std::string& input) {
    std::string buf;
    size_t total = input.size();
    size_t offset = 0;

    while (offset + BLOCK_BYTES <= total) {
        buf.assign(input, offset, BLOCK_BYTES);
        uint32_t block[BLOCK_INTS];
        buf_to_block(buf, block);
        transform(digest, block);
        offset += BLOCK_BYTES;
    }

    // Remaining bytes + padding
    buf.assign(input, offset, total - offset);
    size_t remaining = buf.size();
    buf += (char)0x80;

    // If remaining + 1 (0x80) + 8 (length) > 64, need two blocks
    if (remaining >= BLOCK_BYTES - 8) {
        while (buf.size() < BLOCK_BYTES)
            buf += (char)0x00;
        uint32_t block[BLOCK_INTS];
        buf_to_block(buf, block);
        transform(digest, block);
        buf.clear();
    }
    while (buf.size() < BLOCK_BYTES)
        buf += (char)0x00;

    uint32_t block[BLOCK_INTS];
    buf_to_block(buf, block);
    uint64_t total_bits = total * 8;
    block[BLOCK_INTS - 1] = (uint32_t)total_bits;
    block[BLOCK_INTS - 2] = (uint32_t)(total_bits >> 32);
    transform(digest, block);
}

} // namespace detail::sha1

/// Compute SHA-1 hash of input, returned as 40-char hex string.
inline std::string sha1(const std::string& input) {
    uint32_t digest[5] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
    detail::sha1::compute(digest, input);

    std::ostringstream result;
    for (auto val : digest)
        result << std::hex << std::setfill('0') << std::setw(8) << val;
    return result.str();
}

/// Compute SHA-1 hash, returned as 20 raw bytes.
inline std::vector<uint8_t> sha1_raw(const std::string& input) {
    uint32_t digest[5] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
    detail::sha1::compute(digest, input);

    std::vector<uint8_t> result(20);
    for (int i = 0; i < 5; i++) {
        result[4 * i + 0] = (uint8_t)(digest[i] >> 24);
        result[4 * i + 1] = (uint8_t)(digest[i] >> 16);
        result[4 * i + 2] = (uint8_t)(digest[i] >> 8);
        result[4 * i + 3] = (uint8_t)(digest[i]);
    }
    return result;
}

// -- SHA-256 -----------------------------------------------------------------

namespace detail::sha256 {

static constexpr size_t BLOCK_INTS = 16;
static constexpr size_t BLOCK_BYTES = BLOCK_INTS * 4;

static constexpr uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

inline uint32_t ror(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

inline uint32_t sigma0(uint32_t x) {
    return ror(x, 2) ^ ror(x, 13) ^ ror(x, 22);
}

inline uint32_t sigma1(uint32_t x) {
    return ror(x, 6) ^ ror(x, 11) ^ ror(x, 25);
}

inline uint32_t gamma0(uint32_t x) {
    return ror(x, 7) ^ ror(x, 18) ^ (x >> 3);
}

inline uint32_t gamma1(uint32_t x) {
    return ror(x, 17) ^ ror(x, 19) ^ (x >> 10);
}

inline void buf_to_block(const std::string& buf, uint32_t block[BLOCK_INTS]) {
    for (size_t i = 0; i < BLOCK_INTS; i++) {
        block[i] = (uint32_t)(uint8_t)buf[4 * i + 0] << 24 | (uint32_t)(uint8_t)buf[4 * i + 1] << 16 |
                   (uint32_t)(uint8_t)buf[4 * i + 2] << 8 | (uint32_t)(uint8_t)buf[4 * i + 3];
    }
}

inline void transform(uint32_t state[8], const uint32_t block[BLOCK_INTS]) {
    uint32_t w[64];
    for (int i = 0; i < 16; i++)
        w[i] = block[i];
    for (int i = 16; i < 64; i++)
        w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];

    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + sigma1(e) + ch(e, f, g) + K[i] + w[i];
        uint32_t t2 = sigma0(a) + maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

inline void compute(uint32_t state[8], const std::string& input) {
    std::string buf;
    size_t total = input.size();
    size_t offset = 0;

    while (offset + BLOCK_BYTES <= total) {
        buf.assign(input, offset, BLOCK_BYTES);
        uint32_t block[BLOCK_INTS];
        buf_to_block(buf, block);
        transform(state, block);
        offset += BLOCK_BYTES;
    }

    buf.assign(input, offset, total - offset);
    size_t remaining = buf.size();
    buf += (char)0x80;

    if (remaining >= BLOCK_BYTES - 8) {
        while (buf.size() < BLOCK_BYTES)
            buf += (char)0x00;
        uint32_t block[BLOCK_INTS];
        buf_to_block(buf, block);
        transform(state, block);
        buf.clear();
    }
    while (buf.size() < BLOCK_BYTES)
        buf += (char)0x00;

    uint32_t block[BLOCK_INTS];
    buf_to_block(buf, block);
    uint64_t total_bits = total * 8;
    block[BLOCK_INTS - 1] = (uint32_t)total_bits;
    block[BLOCK_INTS - 2] = (uint32_t)(total_bits >> 32);
    transform(state, block);
}

} // namespace detail::sha256

/// Compute SHA-256 hash of input, returned as 64-char hex string.
inline std::string sha256(const std::string& input) {
    uint32_t state[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                         0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    detail::sha256::compute(state, input);

    std::ostringstream result;
    for (auto val : state)
        result << std::hex << std::setfill('0') << std::setw(8) << val;
    return result.str();
}

/// Compute SHA-256 hash, returned as 32 raw bytes.
inline std::vector<uint8_t> sha256_raw(const std::string& input) {
    uint32_t state[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                         0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    detail::sha256::compute(state, input);

    std::vector<uint8_t> result(32);
    for (int i = 0; i < 8; i++) {
        result[4 * i + 0] = (uint8_t)(state[i] >> 24);
        result[4 * i + 1] = (uint8_t)(state[i] >> 16);
        result[4 * i + 2] = (uint8_t)(state[i] >> 8);
        result[4 * i + 3] = (uint8_t)(state[i]);
    }
    return result;
}

} // namespace crypto
