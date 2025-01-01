#include "Base64.h"

#include <stdexcept>
#include <unordered_map>
#include <vector>

// todo: validate this, it probably sucks
std::string Base64::encode(std::span<const uint8_t> data) {
    static constexpr char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                           "abcdefghijklmnopqrstuvwxyz"
                                           "0123456789+/";

    std::string encoded;
    encoded.reserve((data.size() + 2) / 3 * 4);

    size_t i = 0;
    while (i < data.size()) {
        uint32_t octet_a = i < data.size() ? data[i++] : 0;
        uint32_t octet_b = i < data.size() ? data[i++] : 0;
        uint32_t octet_c = i < data.size() ? data[i++] : 0;

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        encoded.push_back(base64_chars[(triple >> 18) & 0x3F]);
        encoded.push_back(base64_chars[(triple >> 12) & 0x3F]);
        encoded.push_back((i - 1) < data.size() ? base64_chars[(triple >> 6) & 0x3F] : '=');
        encoded.push_back(i < data.size() ? base64_chars[triple & 0x3F] : '=');
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