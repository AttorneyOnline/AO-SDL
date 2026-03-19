#pragma once

#include "Asset.h"

#include <cstdint>
#include <vector>

// A single decoded frame of an image or animation.
// Pixels are always RGBA, top-left origin, tightly packed (width * height * 4 bytes).
struct ImageFrame {
    std::vector<uint8_t> pixels;
    int width;
    int height;
    int duration_ms; // 0 for static images, >0 for animation frames
};

// A decoded image asset. Holds one frame for static images, multiple for animations.
// Pixel data is fully composited RGBA — the renderer can upload directly to a texture.
class ImageAsset : public Asset {
  public:
    ImageAsset(std::string path, std::string format, std::vector<ImageFrame> frames)
        : Asset(std::move(path), std::move(format)), m_frames(std::move(frames)) {}

    bool is_animated() const { return m_frames.size() > 1; }
    int frame_count() const { return static_cast<int>(m_frames.size()); }

    const ImageFrame& frame(int index) const { return m_frames.at(index); }

    int width() const { return m_frames.empty() ? 0 : m_frames[0].width; }
    int height() const { return m_frames.empty() ? 0 : m_frames[0].height; }

    size_t memory_size() const override {
        size_t total = 0;
        for (const auto& f : m_frames) {
            total += f.pixels.size();
        }
        return total;
    }

  private:
    std::vector<ImageFrame> m_frames;
};
