/**
 * @file Base64.h
 * @brief Base64 encoding and decoding utilities.
 */
#pragma once

#include <cstdint>
#include <span>
#include <string>

/**
 * @brief Static utility class for Base64 encoding and decoding.
 */
class Base64 {
  public:
    /**
     * @brief Encodes raw binary data to a Base64 string.
     * @param data A span of bytes to encode.
     * @return The Base64-encoded string.
     */
    static std::string encode(std::span<const uint8_t> data);

    /**
     * @brief Decodes a Base64 string to raw bytes.
     *
     * @warning The returned span points into an internal static buffer.
     * The span is invalidated by the next call to decode(). Callers must
     * copy the data before calling decode() again, or the span will dangle.
     *
     * @param input The Base64-encoded string (passed by non-const reference).
     * @return A span over the decoded bytes in an internal buffer.
     */
    static std::span<const uint8_t> decode(std::string& input);
};
