#include "asset/ImageDecoder.h"

#include "stb_image.h"

class GifImageDecoder : public ImageDecoder {
  public:
    std::vector<std::string> extensions() const override { return {"gif"}; }

    std::vector<ImageFrame> decode(const uint8_t* data, size_t size) const override {
        int* delays = nullptr;
        int width, height, frame_count, channels;

        stbi_set_flip_vertically_on_load(true);
        uint8_t* pixels = stbi_load_gif_from_memory(data, (int)size, &delays, &width, &height,
                                                     &frame_count, &channels, 4);
        stbi_set_flip_vertically_on_load(false);

        std::vector<ImageFrame> frames;
        if (!pixels)
            return frames;

        size_t frame_bytes = (size_t)width * height * 4;
        frames.reserve(frame_count);

        for (int i = 0; i < frame_count; i++) {
            ImageFrame f;
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
