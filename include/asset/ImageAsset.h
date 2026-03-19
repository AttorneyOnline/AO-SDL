/**
 * @file ImageAsset.h
 * @brief Decoded image/animation asset with RGBA pixel frames.
 */
#pragma once

#include "Asset.h"

#include <cstdint>
#include <vector>

/**
 * @brief A single decoded frame of an image or animation.
 *
 * Pixels are always RGBA, top-left origin, tightly packed (width * height * 4 bytes).
 */
struct ImageFrame {
    std::vector<uint8_t> pixels; /**< Raw RGBA pixel data. */
    int width;                   /**< Frame width in pixels. */
    int height;                  /**< Frame height in pixels. */
    int duration_ms;             /**< Frame duration in milliseconds. 0 for static images, >0 for animation frames. */
};

/**
 * @brief A decoded image asset containing one or more RGBA frames.
 *
 * Holds one frame for static images, multiple for animations (APNG, GIF, WebP).
 * Pixel data is fully composited RGBA -- the renderer can upload directly to a
 * GPU texture without further processing.
 *
 * Pixel data stays in RAM for the lifetime of the asset. The AssetCache manages
 * eviction via LRU + shared_ptr pinning. The GL renderer caches GPU textures
 * via weak_ptr — when the AssetCache evicts an asset, the GPU texture is
 * automatically freed on the next draw sweep.
 */
class ImageAsset : public Asset {
  public:
    ImageAsset(std::string path, std::string format, std::vector<ImageFrame> frames)
        : Asset(std::move(path), std::move(format)), frames(std::move(frames)) {}

    bool is_animated() const { return frames.size() > 1; }
    int frame_count() const { return static_cast<int>(frames.size()); }

    const ImageFrame& frame(int index) const { return frames.at(index); }

    int width() const { return frames.empty() ? 0 : frames[0].width; }
    int height() const { return frames.empty() ? 0 : frames[0].height; }

    size_t memory_size() const override {
        size_t total = 0;
        for (const auto& f : frames) {
            total += f.pixels.size();
        }
        return total;
    }

  private:
    std::vector<ImageFrame> frames;
};
