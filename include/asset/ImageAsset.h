/**
 * @file ImageAsset.h
 * @brief Decoded image/animation asset with RGBA pixel frames.
 */
#pragma once

#include "AlignedBuffer.h"
#include "Asset.h"

#include <cstdint>
#include <cstring>
#include <vector>

/**
 * @brief A single decoded frame produced by an image decoder.
 *
 * Decoders fill this struct with RGBA pixel data. The ImageAsset constructor
 * packs the pixel data from all frames into a single contiguous page-aligned
 * buffer, enabling zero-copy GPU texture creation on Apple Silicon.
 */
struct DecodedFrame {
    std::vector<uint8_t> pixels; /**< Raw RGBA pixel data (width * height * 4 bytes). */
    int width;
    int height;
    int duration_ms; /**< 0 for static images, >0 for animation frames. */
};

/**
 * @brief Metadata for a single frame within an ImageAsset.
 *
 * Does not own pixel data — pixels live in the parent ImageAsset's
 * contiguous AlignedBuffer and are accessed via ImageAsset::frame_pixels().
 */
struct ImageFrame {
    int width;
    int height;
    int duration_ms;
};

/**
 * @brief A decoded image asset containing one or more RGBA frames.
 *
 * Pixel data for all frames is stored in a single contiguous page-aligned
 * buffer. On Apple Silicon, the Metal renderer can wrap this buffer with
 * newBufferWithBytesNoCopy to create GPU textures that share the same
 * physical memory — eliminating the ~232 MB GPU-side duplication.
 *
 * The AssetCache manages eviction via LRU + shared_ptr pinning.
 */
class ImageAsset : public Asset {
  public:
    ImageAsset(std::string path, std::string format, std::vector<DecodedFrame> decoded_frames)
        : Asset(std::move(path), std::move(format)) {
        if (decoded_frames.empty())
            return;

        // Compute per-frame byte size (all frames share the same dimensions).
        int w = decoded_frames[0].width;
        int h = decoded_frames[0].height;
        size_t frame_bytes = static_cast<size_t>(w) * h * 4;

        // Pack all frames contiguously into one page-aligned buffer.
        pixel_data_ = AlignedBuffer(frame_bytes * decoded_frames.size());

        for (size_t i = 0; i < decoded_frames.size(); i++) {
            auto& df = decoded_frames[i];
            frames_.push_back({df.width, df.height, df.duration_ms});

            size_t copy_bytes = std::min(df.pixels.size(), frame_bytes);
            std::memcpy(pixel_data_.data() + i * frame_bytes, df.pixels.data(), copy_bytes);
        }
    }

    bool is_animated() const {
        return frames_.size() > 1;
    }
    int frame_count() const {
        return static_cast<int>(frames_.size());
    }

    const ImageFrame& frame(int index) const {
        return frames_.at(index);
    }

    /// Pointer to RGBA pixel data for a specific frame.
    const uint8_t* frame_pixels(int index) const {
        size_t offset = 0;
        for (int i = 0; i < index; i++)
            offset += static_cast<size_t>(frames_[i].width) * frames_[i].height * 4;
        return pixel_data_.data() + offset;
    }

    /// Mutable pointer (for GlyphCache atlas updates).
    uint8_t* frame_pixels_mut(int index) {
        size_t offset = 0;
        for (int i = 0; i < index; i++)
            offset += static_cast<size_t>(frames_[i].width) * frames_[i].height * 4;
        return pixel_data_.data() + offset;
    }

    int width() const {
        return frames_.empty() ? 0 : frames_[0].width;
    }
    int height() const {
        return frames_.empty() ? 0 : frames_[0].height;
    }

    /// Replace pixel data for a single frame (used by GlyphCache atlas updates).
    /// Handles resizing when the incoming data is larger than the current frame
    /// (e.g., GlyphCache atlas grew from 256x256 to 256x512).
    void update_frame(int index, const std::vector<uint8_t>& pixels) {
        if (index < 0 || index >= static_cast<int>(frames_.size()) || pixels.empty())
            return;

        // Detect if the atlas dimensions changed by comparing pixel data sizes.
        // The width stays constant (atlas width is fixed); only height can grow.
        int w = frames_[index].width;
        int new_h = static_cast<int>(pixels.size() / 4) / w;
        if (new_h != frames_[index].height) {
            frames_[index].height = new_h;
        }

        // Recompute total buffer size needed with updated dimensions.
        size_t frame_bytes = static_cast<size_t>(w) * new_h * 4;
        size_t needed = frame_bytes; // single-frame asset (atlas)
        for (int i = 0; i < static_cast<int>(frames_.size()); i++) {
            if (i != index)
                needed += static_cast<size_t>(frames_[i].width) * frames_[i].height * 4;
        }

        if (needed > pixel_data_.size()) {
            AlignedBuffer new_buf(needed);
            // Don't copy old data — we'll write the full new pixels below.
            pixel_data_ = std::move(new_buf);
        }

        // For single-frame assets (atlas), offset is always 0.
        size_t offset = 0;
        for (int i = 0; i < index; i++)
            offset += static_cast<size_t>(frames_[i].width) * frames_[i].height * 4;

        std::memcpy(pixel_data_.data() + offset, pixels.data(), pixels.size());
        generation_++;
    }

    uint64_t generation() const {
        return generation_;
    }

    /// Contiguous pixel buffer (all frames packed sequentially).
    const AlignedBuffer& pixel_data() const {
        return pixel_data_;
    }

    /// Bytes for the first frame (width * height * 4).
    /// For uniform-sized animations, all frames have this size.
    size_t bytes_per_frame() const {
        if (frames_.empty())
            return 0;
        return static_cast<size_t>(frames_[0].width) * frames_[0].height * 4;
    }

    size_t memory_size() const override {
        return pixel_data_.size();
    }

  private:
    std::vector<ImageFrame> frames_;
    AlignedBuffer pixel_data_;
    uint64_t generation_ = 0;
};
