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
 */
class ImageAsset : public Asset {
  public:
    /**
     * @brief Construct an ImageAsset from decoded frames.
     * @param path Virtual asset path (no extension).
     * @param format Resolved format extension (e.g. "png", "apng", "webp").
     * @param frames Vector of decoded image frames (moved).
     */
    ImageAsset(std::string path, std::string format, std::vector<ImageFrame> frames)
        : Asset(std::move(path), std::move(format)), frames(std::move(frames)) {}

    /**
     * @brief Check whether this image has multiple frames (i.e. is an animation).
     * @return True if the image contains more than one frame.
     */
    bool is_animated() const { return frames.size() > 1; }

    /**
     * @brief Get the total number of frames.
     * @return The frame count (1 for static images).
     */
    int frame_count() const { return static_cast<int>(frames.size()); }

    /**
     * @brief Access a frame by index.
     * @param index Zero-based frame index.
     * @return Const reference to the requested ImageFrame.
     * @throws std::out_of_range if index is out of bounds.
     */
    const ImageFrame& frame(int index) const { return frames.at(index); }

    /**
     * @brief Get the width of the first frame (or 0 if empty).
     * @return Width in pixels.
     */
    int width() const { return frames.empty() ? 0 : frames[0].width; }

    /**
     * @brief Get the height of the first frame (or 0 if empty).
     * @return Height in pixels.
     */
    int height() const { return frames.empty() ? 0 : frames[0].height; }

    /**
     * @brief Calculate the total memory footprint of all decoded pixel data.
     * @return Sum of all frames' pixel buffer sizes in bytes.
     */
    size_t memory_size() const override {
        size_t total = 0;
        for (const auto& f : frames) {
            total += f.pixels.size();
        }
        return total;
    }

  private:
    std::vector<ImageFrame> frames; /**< Decoded image frames. */
};
