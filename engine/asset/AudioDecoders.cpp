#include "asset/AudioDecoder.h"

#include <algorithm>

// Defined in formats/OpusDecoder.cpp
std::unique_ptr<AudioDecoder> create_opus_decoder();

// Defined in formats/MiniaudioDecoder.cpp
std::unique_ptr<AudioDecoder> create_miniaudio_decoder();

const std::vector<std::unique_ptr<AudioDecoder>>& audio_decoders() {
    static auto decoders = []() {
        std::vector<std::unique_ptr<AudioDecoder>> v;
        v.push_back(create_opus_decoder());      // Opus via libopusfile (highest priority)
        v.push_back(create_miniaudio_decoder()); // WAV, MP3, FLAC, OGG Vorbis via miniaudio
        return v;
    }();
    return decoders;
}

std::vector<std::string> supported_audio_extensions() {
    std::vector<std::string> exts;
    for (const auto& decoder : audio_decoders()) {
        for (const auto& ext : decoder->extensions()) {
            if (std::find(exts.begin(), exts.end(), ext) == exts.end())
                exts.push_back(ext);
        }
    }
    return exts;
}
