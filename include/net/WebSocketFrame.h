/**
 * @file WebSocketFrame.h
 * @brief WebSocket frame types and wire-format utilities (RFC 6455).
 *
 * Shared by both client and server WebSocket implementations.
 */
#pragma once

#include <bit>
#include <cstdint>
#include <cstring>
#include <vector>

// Byte swap macro for 64-bit network byte order conversion.
#if defined(__GNUC__) || defined(__clang__)
#define BSWAP64(x) __builtin_bswap64(x)
#elif defined(_MSC_VER)
#include <cstdlib>
#define BSWAP64(x) _byteswap_uint64(x)
#else
static inline uint64_t BSWAP64(uint64_t x) {
    return ((x & 0x00000000000000FFULL) << 56) | ((x & 0x000000000000FF00ULL) << 40) |
           ((x & 0x0000000000FF0000ULL) << 24) | ((x & 0x00000000FF000000ULL) << 8) |
           ((x & 0x000000FF00000000ULL) >> 8) | ((x & 0x0000FF0000000000ULL) >> 24) |
           ((x & 0x00FF000000000000ULL) >> 40) | ((x & 0xFF00000000000000ULL) >> 56);
}
#endif

/** @brief WebSocket frame opcodes per RFC 6455. */
enum Opcode : uint8_t { CONTINUATION = 0x00, TEXT = 0x01, BINARY = 0x02, CLOSE = 0x08, PING = 0x09, PONG = 0x0A };

/**
 * @brief A single WebSocket frame.
 *
 * Used for both parsing received frames and constructing outgoing frames.
 * The serialize() method produces the wire-format byte sequence.
 */
struct WebSocketFrame {
    bool complete = false;      /**< True if the frame has been fully received. */
    std::vector<uint8_t> bytes; /**< Raw bytes of the frame as received. */

    bool fin = false;     /**< FIN bit: true if this is the final fragment. */
    uint8_t rsv = 0;      /**< RSV bits (reserved, should be 0). */
    Opcode opcode = TEXT; /**< Frame opcode. */

    bool mask = false;    /**< True if the payload is masked. */
    uint8_t len_code = 0; /**< Raw length code from the frame header. */
    uint64_t len = 0;     /**< Actual payload length after decoding. */

    uint32_t mask_key = 0; /**< Masking key (only valid if mask is true). */

    std::vector<uint8_t> data; /**< Unmasked payload data. */

    /**
     * @brief Serialize this frame into a byte buffer suitable for sending.
     * @return The serialized frame bytes.
     */
    std::vector<uint8_t> serialize();
};

/** @brief Convert a 64-bit value from network to host byte order. */
inline uint64_t net_to_host_64(uint64_t net_value) {
    if constexpr (std::endian::native == std::endian::big) {
        return net_value;
    }
    else if constexpr (std::endian::native == std::endian::little) {
        return BSWAP64(net_value);
    }
    else {
        static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little,
                      "Unsupported endianness");
    }
}

/** @brief Convert a 64-bit value from host to network byte order. */
inline uint64_t host_to_net_64(uint64_t host_value) {
    if constexpr (std::endian::native == std::endian::big) {
        return host_value;
    }
    else if constexpr (std::endian::native == std::endian::little) {
        return BSWAP64(host_value);
    }
    else {
        static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little,
                      "Unsupported endianness");
    }
}

/**
 * @brief Apply (or remove) the WebSocket masking transform in-place.
 *
 * XORs each byte with the corresponding byte of the 4-byte mask key.
 * The same function masks and unmasks (XOR is its own inverse).
 */
inline void apply_mask(uint8_t* data, size_t len, uint32_t mask_key) {
    // Extract mask bytes in network byte order (MSB first) without
    // platform-specific intrinsics. This matches the wire layout
    // defined in RFC 6455 §5.3.
    uint8_t key_bytes[4] = {
        static_cast<uint8_t>((mask_key >> 24) & 0xFF),
        static_cast<uint8_t>((mask_key >> 16) & 0xFF),
        static_cast<uint8_t>((mask_key >> 8) & 0xFF),
        static_cast<uint8_t>((mask_key) & 0xFF),
    };
    for (size_t i = 0; i < len; ++i)
        data[i] ^= key_bytes[i % 4];
}
