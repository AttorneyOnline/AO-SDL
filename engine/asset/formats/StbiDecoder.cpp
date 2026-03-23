#include "asset/ImageDecoder.h"

#include "stb_image.h"
#include "utils/ImageOps.h"

class StbiImageDecoder : public ImageDecoder {
  public:
    std::vector<std::string> extensions() const override {
        return {"jpg", "jpeg", "bmp", "tga"};
    }

    std::vector<DecodedFrame> decode(const uint8_t* data, size_t size) const override {
        if (!data || size < 4)
            return {};

        int width, height, channels;
        uint8_t* pixels = stbi_load_from_memory(data, (int)size, &width, &height, &channels, 4);

        std::vector<DecodedFrame> frames;
        if (!pixels)
            return frames;

        // Flip Y manually instead of using stbi_set_flip_vertically_on_load
        // (which is global state and not thread-safe).
        flip_vertical_rgba(pixels, width, height);

        DecodedFrame f;
        f.width = width;
        f.height = height;
        f.duration_ms = 0;
        f.pixels.assign(pixels, pixels + width * height * 4);
        stbi_image_free(pixels);
        frames.push_back(std::move(f));
        return frames;
    }
};

std::unique_ptr<ImageDecoder> create_stbi_decoder() {
    return std::make_unique<StbiImageDecoder>();
}
