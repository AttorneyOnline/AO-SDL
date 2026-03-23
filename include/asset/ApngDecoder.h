#pragma once

#include "asset/ImageAsset.h"

#include <cstdint>
#include <optional>
#include <vector>

/// Lightweight APNG frame extractor.
///
/// Parses the APNG chunk structure (acTL, fcTL, fdAT) and reconstructs
/// individual PNG images for each frame, which are then decoded by stb_image.
/// Falls back to single-frame PNG decoding if the file is not an APNG.
///
/// Handles:
/// - Full-size frames (same dimensions as canvas)
/// - Sub-frames composited onto the canvas (APNG_DISPOSE_OP_NONE, BACKGROUND, PREVIOUS)
/// - Frame durations from fcTL chunks
///
/// Does NOT handle:
/// - APNG blend modes (treats everything as SOURCE for now)
namespace ApngDecoder {

/// Decode an APNG (or plain PNG) buffer into a list of RGBA frames.
/// Each frame is composited to the full canvas size.
/// Returns nullopt if the data is not a valid PNG.
std::optional<std::vector<DecodedFrame>> decode(const uint8_t* data, size_t size, bool flip_y);

} // namespace ApngDecoder
