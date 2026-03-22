#include "asset/ImageDecoder.h"

#include "stb_image.h"

class StbiImageDecoder : public ImageDecoder {
  public:
    std::vector<std::string> extensions() const override {
        return {"jpg", "jpeg", "bmp", "tga"};
    }

    std::vector<ImageFrame> decode(const uint8_t* data, size_t size) const override {
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);
        uint8_t* pixels = stbi_load_from_memory(data, (int)size, &width, &height, &channels, 4);
        stbi_set_flip_vertically_on_load(false);

        std::vector<ImageFrame> frames;
        if (!pixels)
            return frames;

        ImageFrame f;
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
