#pragma once

#include "Asset.h"

#include <cstdint>
#include <vector>

/**
 * @brief A decoded audio asset containing interleaved float32 PCM samples.
 *
 * Holds fully decoded audio data ready for playback. The AssetCache manages
 * eviction via LRU + shared_ptr pinning, identical to ImageAsset.
 */
class SoundAsset : public Asset {
  public:
    SoundAsset(std::string path, std::string format, uint32_t sample_rate, uint32_t channels,
               std::vector<float> samples)
        : Asset(std::move(path), std::move(format)), sample_rate_(sample_rate), channels_(channels),
          samples_(std::move(samples)) {
    }

    uint32_t sample_rate() const {
        return sample_rate_;
    }
    uint32_t channels() const {
        return channels_;
    }

    const float* data() const {
        return samples_.data();
    }
    size_t sample_count() const {
        return samples_.size();
    }

    /// Total number of frames (sample_count / channels).
    size_t frame_count() const {
        return channels_ > 0 ? samples_.size() / channels_ : 0;
    }

    float duration_seconds() const {
        return (sample_rate_ > 0 && channels_ > 0) ? static_cast<float>(samples_.size()) / (channels_ * sample_rate_)
                                                   : 0.0f;
    }

    size_t memory_size() const override {
        return samples_.size() * sizeof(float);
    }

  private:
    uint32_t sample_rate_;
    uint32_t channels_;
    std::vector<float> samples_;
};
