#include "asset/AudioDecoder.h"
#include "utils/Log.h"

#include <opusfile.h>

namespace {

class OpusFileDecoder : public AudioDecoder {
  public:
    std::vector<std::string> extensions() const override {
        return {"opus"};
    }

    std::unique_ptr<SoundAsset> decode(const std::string& path, const uint8_t* data, size_t size) const override {
        int error = 0;
        OggOpusFile* of = op_open_memory(data, size, &error);
        if (!of) {
            Log::log_print(DEBUG, "OpusDecoder: open failed for '%s' (error %d)", path.c_str(), error);
            return nullptr;
        }

        // Opus always decodes to 48000 Hz
        constexpr uint32_t SAMPLE_RATE = 48000;
        constexpr int CHANNELS = 2;

        // Get total sample count if available
        ogg_int64_t total_pcm = op_pcm_total(of, -1);

        std::vector<float> samples;
        if (total_pcm > 0) {
            samples.reserve(total_pcm * CHANNELS);
        }

        // Decode in chunks
        float buf[5760 * 2]; // max opus frame size * stereo
        for (;;) {
            int ret = op_read_float_stereo(of, buf, sizeof(buf) / sizeof(buf[0]));
            if (ret <= 0)
                break;
            samples.insert(samples.end(), buf, buf + ret * CHANNELS);
        }

        op_free(of);

        if (samples.empty())
            return nullptr;

        return std::make_unique<SoundAsset>(path, "opus", SAMPLE_RATE, CHANNELS, std::move(samples));
    }
};

} // namespace

std::unique_ptr<AudioDecoder> create_opus_decoder() {
    return std::make_unique<OpusFileDecoder>();
}
