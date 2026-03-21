#include "asset/ImageDecoder.h"

#include <unordered_set>

// Factory functions defined in asset/formats/*.cpp
std::unique_ptr<ImageDecoder> create_webp_decoder();
std::unique_ptr<ImageDecoder> create_apng_decoder();
std::unique_ptr<ImageDecoder> create_gif_decoder();
std::unique_ptr<ImageDecoder> create_stbi_decoder();

const std::vector<std::unique_ptr<ImageDecoder>>& image_decoders() {
    static std::vector<std::unique_ptr<ImageDecoder>> decoders = []() {
        std::vector<std::unique_ptr<ImageDecoder>> v;
        v.push_back(create_webp_decoder());
        v.push_back(create_apng_decoder());
        v.push_back(create_gif_decoder());
        v.push_back(create_stbi_decoder());
        return v;
    }();
    return decoders;
}

std::vector<std::string> supported_image_extensions() {
    std::vector<std::string> result;
    std::unordered_set<std::string> seen;
    for (const auto& dec : image_decoders()) {
        for (const auto& ext : dec->extensions()) {
            if (seen.insert(ext).second)
                result.push_back(ext);
        }
    }
    return result;
}
