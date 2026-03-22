#include "asset/AudioDecoder.h"

#include "utils/Log.h"

#define MA_NO_DEVICE_IO
#define MA_NO_GENERATION
#define MA_NO_ENGINE
#define MA_NO_NODE_GRAPH
#define MA_NO_RESOURCE_MANAGER
#include "miniaudio.h"

namespace {

class MiniaudioDecoder : public AudioDecoder {
  public:
    std::vector<std::string> extensions() const override {
        return {"ogg", "mp3", "wav", "flac"};
    }

    std::unique_ptr<SoundAsset> decode(const std::string& path, const uint8_t* data, size_t size) const override {
        ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 48000);
        ma_decoder decoder;

        ma_result result = ma_decoder_init_memory(data, size, &config, &decoder);
        if (result != MA_SUCCESS) {
            Log::log_print(DEBUG, "MiniaudioDecoder: init failed for '%s' (error %d, %zu bytes)", path.c_str(), result,
                           size);
            return nullptr;
        }

        ma_uint64 total_frames;
        result = ma_decoder_get_length_in_pcm_frames(&decoder, &total_frames);
        if (result != MA_SUCCESS || total_frames == 0) {
            // Length unknown — decode in chunks
            total_frames = 0;
        }

        // Decode all frames into a float buffer (2 channels)
        std::vector<float> samples;
        if (total_frames > 0) {
            samples.resize(total_frames * 2);
            ma_uint64 frames_read;
            result = ma_decoder_read_pcm_frames(&decoder, samples.data(), total_frames, &frames_read);
            samples.resize(frames_read * 2);
        }
        else {
            // Streaming decode for unknown-length files
            constexpr size_t CHUNK_FRAMES = 16384;
            std::vector<float> chunk(CHUNK_FRAMES * 2);
            for (;;) {
                ma_uint64 frames_read;
                result = ma_decoder_read_pcm_frames(&decoder, chunk.data(), CHUNK_FRAMES, &frames_read);
                if (frames_read == 0)
                    break;
                samples.insert(samples.end(), chunk.begin(), chunk.begin() + frames_read * 2);
                if (frames_read < CHUNK_FRAMES)
                    break;
            }
        }

        ma_decoder_uninit(&decoder);

        if (samples.empty())
            return nullptr;

        std::string format = path.substr(path.rfind('.') + 1);
        return std::make_unique<SoundAsset>(path, format, 48000, 2, std::move(samples));
    }
};

} // namespace

std::unique_ptr<AudioDecoder> create_miniaudio_decoder() {
    return std::make_unique<MiniaudioDecoder>();
}
