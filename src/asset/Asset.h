#pragma once

#include <cstddef>
#include <string>

class Asset {
  public:
    // Virtual path used to look this asset up (no extension).
    // e.g. "characters/Phoenix/normal"
    const std::string& path() const { return m_path; }

    // Resolved format extension (no dot): "png", "apng", "webp", "opus", etc.
    // Empty if the asset was fetched with an explicit extension via raw().
    const std::string& format() const { return m_format; }

    // Approximate size of this asset's decoded data in bytes.
    // Used by AssetCache for memory accounting.
    virtual size_t memory_size() const = 0;

    virtual ~Asset() = default;

  protected:
    Asset(std::string path, std::string format)
        : m_path(std::move(path)), m_format(std::move(format)) {}

  private:
    std::string m_path;
    std::string m_format;
};
