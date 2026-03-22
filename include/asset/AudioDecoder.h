#pragma once

#include "asset/SoundAsset.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/// Abstract interface for audio format decoders.
///
/// Each decoder handles one or more file extensions and converts raw
/// bytes into a decoded SoundAsset with float32 PCM samples.
class AudioDecoder {
  public:
    virtual ~AudioDecoder() = default;

    /// File extensions this decoder handles (without dots), e.g. {"mp3", "wav"}.
    virtual std::vector<std::string> extensions() const = 0;

    /// Attempt to decode raw audio data into a SoundAsset.
    /// Returns nullptr if the data is not valid for this format.
    virtual std::unique_ptr<SoundAsset> decode(const std::string& path, const uint8_t* data, size_t size) const = 0;
};

/// Returns the global ordered list of audio decoders.
/// Decoders are tried in registration order; the first one that succeeds wins.
const std::vector<std::unique_ptr<AudioDecoder>>& audio_decoders();

/// Collect all supported extensions from all registered decoders (deduplicated).
std::vector<std::string> supported_audio_extensions();
