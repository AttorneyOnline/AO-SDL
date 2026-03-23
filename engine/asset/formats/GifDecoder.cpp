#include "asset/ImageDecoder.h"

#include "stb_image.h"
#include "utils/ImageOps.h"

class GifImageDecoder : public ImageDecoder {
  public:
    std::vector<std::string> extensions() const override {
        return {"gif"};
    }

    std::vector<DecodedFrame> decode(const uint8_t* data, size_t size) const override {
        // stbi_load_gif_from_memory can crash on very short or non-GIF input
        // (especially on macOS). Validate the GIF87a/GIF89a header first.
        if (!data || size < 13)
            return {};
        if (data[0] != 'G' || data[1] != 'I' || data[2] != 'F')
            return {};

        int* delays = nullptr;
        int width, height, frame_count, channels;

        uint8_t* pixels =
            stbi_load_gif_from_memory(data, (int)size, &delays, &width, &height, &frame_count, &channels, 4);

        std::vector<DecodedFrame> frames;
        if (!pixels)
            return frames;

        size_t frame_bytes = (size_t)width * height * 4;
        frames.reserve(frame_count);

        // Flip each frame manually instead of using stbi_set_flip_vertically_on_load
        // (which is global state and not thread-safe).
        for (int i = 0; i < frame_count; i++) {
            flip_vertical_rgba(pixels + i * frame_bytes, width, height);
            DecodedFrame f;
            f.width = width;
            f.height = height;
            f.duration_ms = delays ? delays[i] : 100;
            f.pixels.assign(pixels + i * frame_bytes, pixels + (i + 1) * frame_bytes);
            frames.push_back(std::move(f));
        }

        stbi_image_free(pixels);
        if (delays)
            stbi_image_free(delays);
        return frames;
    }
};

std::unique_ptr<ImageDecoder> create_gif_decoder() {
    return std::make_unique<GifImageDecoder>();
}
