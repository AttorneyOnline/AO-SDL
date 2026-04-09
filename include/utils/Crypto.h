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

#ifndef _WIN32
#include <stdlib.h> // arc4random_buf (macOS, Linux glibc 2.36+)
#endif

namespace crypto {

// -- Shared helpers ----------------------------------------------------------

namespace detail {

inline uint32_t read_be32(const uint8_t* p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | (uint32_t)p[3];
}

inline void write_be32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)(v);
}

inline std::string to_hex(const uint32_t* words, size_t count) {
    std::ostringstream ss;
    for (size_t i = 0; i < count; i++)
        ss << std::hex << std::setfill('0') << std::setw(8) << words[i];
    return ss.str();
}

inline std::vector<uint8_t> to_bytes(const uint32_t* words, size_t count) {
    std::vector<uint8_t> out(count * 4);
    for (size_t i = 0; i < count; i++)
        write_be32(out.data() + 4 * i, words[i]);
    return out;
}

} // namespace detail

// -- SHA-1 -------------------------------------------------------------------

namespace detail::sha1 {

static constexpr size_t BLOCK = 64;

inline uint32_t rol(uint32_t v, uint32_t n) {
    return (v << n) | (v >> (32 - n));
}

inline uint32_t blk(const uint32_t w[16], size_t i) {
    return rol(w[(i + 13) & 15] ^ w[(i + 8) & 15] ^ w[(i + 2) & 15] ^ w[i], 1);
}

inline void R0(const uint32_t w[16], uint32_t v, uint32_t& a, uint32_t x, uint32_t y, uint32_t& z, size_t i) {
    z += ((a & (x ^ y)) ^ y) + w[i] + 0x5a827999 + rol(v, 5);
    a = rol(a, 30);
}
inline void R1(uint32_t w[16], uint32_t v, uint32_t& a, uint32_t x, uint32_t y, uint32_t& z, size_t i) {
    w[i] = blk(w, i);
    z += ((a & (x ^ y)) ^ y) + w[i] + 0x5a827999 + rol(v, 5);
    a = rol(a, 30);
}
inline void R2(uint32_t w[16], uint32_t v, uint32_t& a, uint32_t x, uint32_t y, uint32_t& z, size_t i) {
    w[i] = blk(w, i);
    z += (a ^ x ^ y) + w[i] + 0x6ed9eba1 + rol(v, 5);
    a = rol(a, 30);
}
inline void R3(uint32_t w[16], uint32_t v, uint32_t& a, uint32_t x, uint32_t y, uint32_t& z, size_t i) {
    w[i] = blk(w, i);
    z += (((a | x) & y) | (a & x)) + w[i] + 0x8f1bbcdc + rol(v, 5);
    a = rol(a, 30);
}
inline void R4(uint32_t w[16], uint32_t v, uint32_t& a, uint32_t x, uint32_t y, uint32_t& z, size_t i) {
    w[i] = blk(w, i);
    z += (a ^ x ^ y) + w[i] + 0xca62c1d6 + rol(v, 5);
    a = rol(a, 30);
}

// Copied verbatim from the original sha1.hpp (public domain).
// 4 rounds of 20 operations each, loop unrolled.
inline void transform(uint32_t h[5], uint32_t w[16]) {
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];

    R0(w, a, b, c, d, e, 0);
    R0(w, e, a, b, c, d, 1);
    R0(w, d, e, a, b, c, 2);
    R0(w, c, d, e, a, b, 3);
    R0(w, b, c, d, e, a, 4);
    R0(w, a, b, c, d, e, 5);
    R0(w, e, a, b, c, d, 6);
    R0(w, d, e, a, b, c, 7);
    R0(w, c, d, e, a, b, 8);
    R0(w, b, c, d, e, a, 9);
    R0(w, a, b, c, d, e, 10);
    R0(w, e, a, b, c, d, 11);
    R0(w, d, e, a, b, c, 12);
    R0(w, c, d, e, a, b, 13);
    R0(w, b, c, d, e, a, 14);
    R0(w, a, b, c, d, e, 15);
    R1(w, e, a, b, c, d, 0);
    R1(w, d, e, a, b, c, 1);
    R1(w, c, d, e, a, b, 2);
    R1(w, b, c, d, e, a, 3);
    R2(w, a, b, c, d, e, 4);
    R2(w, e, a, b, c, d, 5);
    R2(w, d, e, a, b, c, 6);
    R2(w, c, d, e, a, b, 7);
    R2(w, b, c, d, e, a, 8);
    R2(w, a, b, c, d, e, 9);
    R2(w, e, a, b, c, d, 10);
    R2(w, d, e, a, b, c, 11);
    R2(w, c, d, e, a, b, 12);
    R2(w, b, c, d, e, a, 13);
    R2(w, a, b, c, d, e, 14);
    R2(w, e, a, b, c, d, 15);
    R2(w, d, e, a, b, c, 0);
    R2(w, c, d, e, a, b, 1);
    R2(w, b, c, d, e, a, 2);
    R2(w, a, b, c, d, e, 3);
    R2(w, e, a, b, c, d, 4);
    R2(w, d, e, a, b, c, 5);
    R2(w, c, d, e, a, b, 6);
    R2(w, b, c, d, e, a, 7);
    R3(w, a, b, c, d, e, 8);
    R3(w, e, a, b, c, d, 9);
    R3(w, d, e, a, b, c, 10);
    R3(w, c, d, e, a, b, 11);
    R3(w, b, c, d, e, a, 12);
    R3(w, a, b, c, d, e, 13);
    R3(w, e, a, b, c, d, 14);
    R3(w, d, e, a, b, c, 15);
    R3(w, c, d, e, a, b, 0);
    R3(w, b, c, d, e, a, 1);
    R3(w, a, b, c, d, e, 2);
    R3(w, e, a, b, c, d, 3);
    R3(w, d, e, a, b, c, 4);
    R3(w, c, d, e, a, b, 5);
    R3(w, b, c, d, e, a, 6);
    R3(w, a, b, c, d, e, 7);
    R3(w, e, a, b, c, d, 8);
    R3(w, d, e, a, b, c, 9);
    R3(w, c, d, e, a, b, 10);
    R3(w, b, c, d, e, a, 11);
    R4(w, a, b, c, d, e, 12);
    R4(w, e, a, b, c, d, 13);
    R4(w, d, e, a, b, c, 14);
    R4(w, c, d, e, a, b, 15);
    R4(w, b, c, d, e, a, 0);
    R4(w, a, b, c, d, e, 1);
    R4(w, e, a, b, c, d, 2);
    R4(w, d, e, a, b, c, 3);
    R4(w, c, d, e, a, b, 4);
    R4(w, b, c, d, e, a, 5);
    R4(w, a, b, c, d, e, 6);
    R4(w, e, a, b, c, d, 7);
    R4(w, d, e, a, b, c, 8);
    R4(w, c, d, e, a, b, 9);
    R4(w, b, c, d, e, a, 10);
    R4(w, a, b, c, d, e, 11);
    R4(w, e, a, b, c, d, 12);
    R4(w, d, e, a, b, c, 13);
    R4(w, c, d, e, a, b, 14);
    R4(w, b, c, d, e, a, 15);

    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
}

inline void load_block(const uint8_t* src, uint32_t w[16]) {
    for (int i = 0; i < 16; i++)
        w[i] = read_be32(src + 4 * i);
}

inline void compute(uint32_t h[5], const uint8_t* data, size_t len) {
    size_t offset = 0;
    uint32_t w[16];

    // Full blocks
    while (offset + BLOCK <= len) {
        load_block(data + offset, w);
        transform(h, w);
        offset += BLOCK;
    }

    // Final padded block(s)
    uint8_t pad[BLOCK * 2];
    size_t remaining = len - offset;
    std::memcpy(pad, data + offset, remaining);
    pad[remaining] = 0x80;
    size_t pad_len = remaining + 1;

    // Determine total padded size: need room for 8-byte length at end
    size_t total_pad = (remaining < BLOCK - 8) ? BLOCK : BLOCK * 2;
    std::memset(pad + pad_len, 0, total_pad - pad_len);

    // Write length in bits as big-endian uint64 at end
    uint64_t bits = (uint64_t)len * 8;
    write_be32(pad + total_pad - 8, (uint32_t)(bits >> 32));
    write_be32(pad + total_pad - 4, (uint32_t)bits);

    load_block(pad, w);
    transform(h, w);
    if (total_pad > BLOCK) {
        load_block(pad + BLOCK, w);
        transform(h, w);
    }
}

} // namespace detail::sha1

/// Compute SHA-1 hash of input, returned as 40-char hex string.
inline std::string sha1(const std::string& input) {
    uint32_t h[5] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
    detail::sha1::compute(h, reinterpret_cast<const uint8_t*>(input.data()), input.size());
    return detail::to_hex(h, 5);
}

/// Compute SHA-1 hash, returned as 20 raw bytes.
inline std::vector<uint8_t> sha1_raw(const std::string& input) {
    uint32_t h[5] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
    detail::sha1::compute(h, reinterpret_cast<const uint8_t*>(input.data()), input.size());
    return detail::to_bytes(h, 5);
}

// -- SHA-256 -----------------------------------------------------------------

namespace detail::sha256 {

static constexpr size_t BLOCK = 64;

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

inline void transform(uint32_t s[8], const uint32_t blk[16]) {
    uint32_t w[64];
    for (int i = 0; i < 16; i++)
        w[i] = blk[i];
    for (int i = 16; i < 64; i++)
        w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];

    uint32_t a = s[0], b = s[1], c = s[2], d = s[3];
    uint32_t e = s[4], f = s[5], g = s[6], h = s[7];

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

    s[0] += a;
    s[1] += b;
    s[2] += c;
    s[3] += d;
    s[4] += e;
    s[5] += f;
    s[6] += g;
    s[7] += h;
}

inline void compute(uint32_t s[8], const uint8_t* data, size_t len) {
    size_t offset = 0;
    uint32_t w[16];

    while (offset + BLOCK <= len) {
        for (int i = 0; i < 16; i++)
            w[i] = read_be32(data + offset + 4 * i);
        transform(s, w);
        offset += BLOCK;
    }

    uint8_t pad[BLOCK * 2];
    size_t remaining = len - offset;
    std::memcpy(pad, data + offset, remaining);
    pad[remaining] = 0x80;
    size_t pad_len = remaining + 1;

    size_t total_pad = (remaining < BLOCK - 8) ? BLOCK : BLOCK * 2;
    std::memset(pad + pad_len, 0, total_pad - pad_len);

    uint64_t bits = (uint64_t)len * 8;
    write_be32(pad + total_pad - 8, (uint32_t)(bits >> 32));
    write_be32(pad + total_pad - 4, (uint32_t)bits);

    for (int i = 0; i < 16; i++)
        w[i] = read_be32(pad + 4 * i);
    transform(s, w);
    if (total_pad > BLOCK) {
        for (int i = 0; i < 16; i++)
            w[i] = read_be32(pad + BLOCK + 4 * i);
        transform(s, w);
    }
}

} // namespace detail::sha256

/// Compute SHA-256 hash of input, returned as 64-char hex string.
inline std::string sha256(const std::string& input) {
    uint32_t s[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    detail::sha256::compute(s, reinterpret_cast<const uint8_t*>(input.data()), input.size());
    return detail::to_hex(s, 8);
}

/// Compute SHA-256 hash of raw bytes, returned as 32 raw bytes.
inline std::vector<uint8_t> sha256_raw(const uint8_t* data, size_t len) {
    uint32_t s[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    detail::sha256::compute(s, data, len);
    return detail::to_bytes(s, 8);
}

/// Compute SHA-256 hash, returned as 32 raw bytes.
inline std::vector<uint8_t> sha256_raw(const std::string& input) {
    return sha256_raw(reinterpret_cast<const uint8_t*>(input.data()), input.size());
}

// -- HMAC-SHA256 --------------------------------------------------------------

/// Compute HMAC-SHA256 as raw 32-byte digest.
/// key and message are arbitrary binary data.
inline std::vector<uint8_t> hmac_sha256(const std::vector<uint8_t>& key, const std::vector<uint8_t>& message) {
    constexpr size_t BLOCK_SIZE = 64; // SHA-256 block size

    // Step 1: If key > block size, hash it. If shorter, zero-pad.
    std::vector<uint8_t> padded_key(BLOCK_SIZE, 0);
    if (key.size() > BLOCK_SIZE) {
        auto hashed = sha256_raw(key.data(), key.size());
        std::copy(hashed.begin(), hashed.end(), padded_key.begin());
    }
    else {
        std::copy(key.begin(), key.end(), padded_key.begin());
    }

    // Step 2: Create inner and outer padded keys
    std::vector<uint8_t> i_key_pad(BLOCK_SIZE);
    std::vector<uint8_t> o_key_pad(BLOCK_SIZE);
    for (size_t i = 0; i < BLOCK_SIZE; i++) {
        i_key_pad[i] = padded_key[i] ^ 0x36;
        o_key_pad[i] = padded_key[i] ^ 0x5c;
    }

    // Step 3: inner hash = SHA256(i_key_pad || message)
    std::vector<uint8_t> inner_data(i_key_pad);
    inner_data.insert(inner_data.end(), message.begin(), message.end());
    auto inner_hash = sha256_raw(inner_data.data(), inner_data.size());

    // Step 4: outer hash = SHA256(o_key_pad || inner_hash)
    std::vector<uint8_t> outer_data(o_key_pad);
    outer_data.insert(outer_data.end(), inner_hash.begin(), inner_hash.end());
    return sha256_raw(outer_data.data(), outer_data.size());
}

/// Convenience: HMAC-SHA256 with string key and message, returns hex string.
inline std::string hmac_sha256_hex(const std::string& key, const std::string& message) {
    std::vector<uint8_t> k(key.begin(), key.end());
    std::vector<uint8_t> m(message.begin(), message.end());
    auto digest = hmac_sha256(k, m);
    // Convert to hex
    uint32_t words[8];
    for (int i = 0; i < 8; i++)
        words[i] = detail::read_be32(digest.data() + 4 * i);
    return detail::to_hex(words, 8);
}

/// Convenience: HMAC-SHA256 with raw key bytes and string message.
inline std::vector<uint8_t> hmac_sha256(const std::vector<uint8_t>& key, const std::string& message) {
    std::vector<uint8_t> m(message.begin(), message.end());
    return hmac_sha256(key, m);
}

// -- PBKDF2-SHA256 ------------------------------------------------------------

/// Derive a key from password and salt using PBKDF2-HMAC-SHA256.
/// Compatible with akashi's CryptoHelper::pbkdf2() (100,000 iterations, 32-byte output).
inline std::vector<uint8_t> pbkdf2_sha256(const std::vector<uint8_t>& password, const std::vector<uint8_t>& salt,
                                          int iterations, size_t output_len = 32) {
    const size_t hash_len = 32; // SHA-256 output
    size_t blocks_needed = (output_len + hash_len - 1) / hash_len;
    std::vector<uint8_t> output;
    output.reserve(blocks_needed * hash_len);

    for (size_t block = 1; block <= blocks_needed; block++) {
        // U_1 = HMAC(password, salt || BE32(block))
        std::vector<uint8_t> salt_block(salt);
        uint8_t be[4];
        detail::write_be32(be, static_cast<uint32_t>(block));
        salt_block.insert(salt_block.end(), be, be + 4);

        auto u = hmac_sha256(password, salt_block);
        auto result = u;

        // U_2 .. U_c
        for (int i = 1; i < iterations; i++) {
            u = hmac_sha256(password, u);
            for (size_t j = 0; j < hash_len; j++)
                result[j] ^= u[j];
        }

        output.insert(output.end(), result.begin(), result.end());
    }

    output.resize(output_len);
    return output;
}

/// Convenience: PBKDF2 with string password and hex-encoded salt, returns hex string.
/// Matches akashi's hash_password() for salt >= 16 bytes.
inline std::string pbkdf2_sha256_hex(const std::string& password, const std::string& hex_salt,
                                     int iterations = 100000) {
    // Decode hex salt to bytes
    std::vector<uint8_t> salt_bytes;
    salt_bytes.reserve(hex_salt.size() / 2);
    for (size_t i = 0; i + 1 < hex_salt.size(); i += 2) {
        unsigned byte = 0;
        for (int j = 0; j < 2; j++) {
            byte <<= 4;
            char c = hex_salt[i + j];
            if (c >= '0' && c <= '9')
                byte |= (c - '0');
            else if (c >= 'a' && c <= 'f')
                byte |= (c - 'a' + 10);
            else if (c >= 'A' && c <= 'F')
                byte |= (c - 'A' + 10);
        }
        salt_bytes.push_back(static_cast<uint8_t>(byte));
    }

    std::vector<uint8_t> pw_bytes(password.begin(), password.end());
    auto derived = pbkdf2_sha256(pw_bytes, salt_bytes, iterations);

    // Convert to hex
    std::ostringstream ss;
    for (auto b : derived)
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(b);
    return ss.str();
}

// -- Secure random bytes ------------------------------------------------------

#ifdef _WIN32
// RtlGenRandom (SystemFunction036) — available on XP+ via advapi32.
// Declared at file scope to avoid pulling in ntsecapi.h / bcrypt.h.
extern "C" __declspec(dllimport) unsigned char __stdcall SystemFunction036(void*, unsigned long);
#endif

/// Generate cryptographically secure random bytes.
inline std::vector<uint8_t> randbytes(size_t count) {
    std::vector<uint8_t> buf(count);
#ifdef _WIN32
    SystemFunction036(buf.data(), static_cast<unsigned long>(count));
#else
    // arc4random_buf is available on macOS and modern Linux (glibc 2.36+)
    arc4random_buf(buf.data(), count);
#endif
    return buf;
}

/// Generate random bytes and return as hex string.
inline std::string randbytes_hex(size_t count) {
    auto bytes = randbytes(count);
    std::ostringstream ss;
    for (auto b : bytes)
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(b);
    return ss.str();
}

} // namespace crypto
