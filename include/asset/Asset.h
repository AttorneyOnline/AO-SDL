/**
 * @file Asset.h
 * @brief Base class for all loaded assets (images, audio, fonts, etc.).
 */
#pragma once

#include <cstddef>
#include <string>

/**
 * @brief Abstract base class for a loaded asset.
 *
 * Every asset has a virtual path (used for cache lookup), a resolved format
 * extension, and a memory size estimate used by AssetCache for eviction
 * accounting.
 */
class Asset {
  public:
    /**
     * @brief Get the virtual path used to look this asset up (no extension).
     *
     * Example: "characters/Phoenix/normal"
     *
     * @return Const reference to the asset's virtual path string.
     */
    const std::string& path() const {
        return path_;
    }

    /**
     * @brief Get the resolved format extension (no leading dot).
     *
     * Examples: "png", "apng", "webp", "opus".
     * Empty if the asset was fetched with an explicit extension via raw().
     *
     * @return Const reference to the format string.
     */
    const std::string& format() const {
        return format_;
    }

    /**
     * @brief Get the approximate size of this asset's decoded data in bytes.
     *
     * Used by AssetCache for memory accounting and eviction decisions.
     *
     * @return Estimated memory footprint in bytes.
     */
    virtual size_t memory_size() const = 0;

    virtual ~Asset() = default;

  protected:
    /**
     * @brief Construct an Asset with a virtual path and format.
     * @param path Virtual path (no extension).
     * @param format Resolved format extension (no dot).
     */
    Asset(std::string path, std::string format) : path_(std::move(path)), format_(std::move(format)) {
    }

  private:
    std::string path_;   /**< Virtual lookup path. */
    std::string format_; /**< Resolved format extension. */
};
