#include "Base64.h"

#include <stdexcept>
#include <unordered_map>
#include <vector>

std::string Base64::encode(std::span<const uint8_t> data) {
    static const char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                         "abcdefghijklmnopqrstuvwxyz"
                                         "0123456789+/";
    static const char padding_char = '=';
    size_t input_length = data.size();
    size_t full_chunks = input_length / 3;
    size_t remaining_bytes = input_length % 3;

    // Calculate output length
    size_t output_length = 4 * ((input_length + 2) / 3);
    std::string encoded;
    encoded.reserve(output_length);

    size_t i = 0;

    // Process full 3-byte chunks
    for (size_t chunk = 0; chunk < full_chunks; ++chunk) {
        uint32_t triple = (data[i] << 16) + (data[i + 1] << 8) + data[i + 2];
        encoded += encoding_table[(triple >> 18) & 0x3F];
        encoded += encoding_table[(triple >> 12) & 0x3F];
        encoded += encoding_table[(triple >> 6) & 0x3F];
        encoded += encoding_table[triple & 0x3F];
        i += 3;
    }

    // Handle remaining bytes and padding
    if (remaining_bytes > 0) {
        uint32_t triple = data[i] << 16;
        if (remaining_bytes == 2) {
            triple += data[i + 1] << 8;
        }

        // Encode first two characters
        encoded += encoding_table[(triple >> 18) & 0x3F];
        encoded += encoding_table[(triple >> 12) & 0x3F];

        if (remaining_bytes == 1) {
            // Only one byte left, pad the last two characters
            encoded += padding_char;
            encoded += padding_char;
        }
        else { // remaining_bytes == 2
            // Encode the third character and pad the fourth
            encoded += encoding_table[(triple >> 6) & 0x3F];
            encoded += padding_char;
        }
    }

    return encoded;
}

std::span<const uint8_t> Base64::decode(std::string& input) {
    static constexpr char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                           "abcdefghijklmnopqrstuvwxyz"
                                           "0123456789+/";

    static const std::unordered_map<char, uint8_t> base64_map = [] {
        std::unordered_map<char, uint8_t> map;
        for (size_t i = 0; i < 64; ++i) {
            map[base64_chars[i]] = static_cast<uint8_t>(i);
        }
        return map;
    }();

    if (input.size() % 4 != 0) {
        throw std::invalid_argument("Base64 input length must be a multiple of 4");
    }

    std::vector<uint8_t> decoded;
    decoded.reserve((input.size() / 4) * 3);

    size_t i = 0;
    while (i < input.size()) {
        uint32_t sextet_a = input[i] != '=' ? base64_map.at(input[i]) : 0;
        uint32_t sextet_b = input[i + 1] != '=' ? base64_map.at(input[i + 1]) : 0;
        uint32_t sextet_c = input[i + 2] != '=' ? base64_map.at(input[i + 2]) : 0;
        uint32_t sextet_d = input[i + 3] != '=' ? base64_map.at(input[i + 3]) : 0;

        uint32_t triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | sextet_d;

        if (input[i + 2] != '=')
            decoded.push_back((triple >> 16) & 0xFF);
        if (input[i + 3] != '=')
            decoded.push_back((triple >> 8) & 0xFF);
        if (input[i + 3] != '=')
            decoded.push_back(triple & 0xFF);

        i += 4;
    }

    return decoded;
}
