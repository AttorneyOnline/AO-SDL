#pragma once

#include <cstdint>
#include <span>
#include <string>

class Base64 {
  public:
    static std::string encode(std::span<const uint8_t> data);
    static std::span<const uint8_t> decode(std::string& input);
};
