#pragma once

#include "asset/ImageAsset.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/// Abstract interface for image format decoders.
///
/// Each decoder handles one or more file extensions and converts raw
/// bytes into a vector of RGBA ImageFrames (with vertical flip for GL/Metal).
class ImageDecoder {
  public:
    virtual ~ImageDecoder() = default;

    /// File extensions this decoder handles (without dots), e.g. {"png", "apng"}.
    virtual std::vector<std::string> extensions() const = 0;

    /// Attempt to decode raw image data into RGBA frames.
    /// Returns an empty vector if the data is not valid for this format.
    virtual std::vector<DecodedFrame> decode(const uint8_t* data, size_t size) const = 0;
};

/// Returns the global ordered list of image decoders.
/// Decoders are tried in registration order; the first one that produces
/// frames wins. The returned vector is built once and cached.
const std::vector<std::unique_ptr<ImageDecoder>>& image_decoders();

/// Collect all supported extensions from all registered decoders (deduplicated).
std::vector<std::string> supported_image_extensions();
