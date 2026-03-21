#pragma once

#include "asset/Asset.h"

#include <cstdint>
#include <vector>

/// Asset wrapping raw bytes (configs, fonts, or other non-decoded data).
/// Stored in AssetCache so all downloaded/loaded data is visible in one place.
class RawAsset : public Asset {
  public:
    RawAsset(std::string path, std::string format, std::vector<uint8_t> data)
        : Asset(std::move(path), std::move(format)), data_(std::move(data)) {}

    size_t memory_size() const override { return data_.size(); }

    const std::vector<uint8_t>& data() const { return data_; }

  private:
    std::vector<uint8_t> data_;
};
