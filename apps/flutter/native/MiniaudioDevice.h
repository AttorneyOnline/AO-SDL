#pragma once

#include "audio/IAudioDevice.h"

#include <miniaudio.h>

#include <array>
#include <mutex>

/**
 * @brief Miniaudio-based audio playback device for mobile platforms.
 *
 * Drop-in replacement for SDLAudioDevice using miniaudio's cross-platform
 * device API (CoreAudio on iOS/macOS, AAudio/OpenSL on Android).
 * Same pull-model callback, same 16-channel mixer, same interface.
 */
class MiniaudioDevice : public IAudioDevice {
  public:
    static constexpr int NUM_CHANNELS = 16;
    static constexpr int SAMPLE_RATE = 48000;

    MiniaudioDevice() = default;
    ~MiniaudioDevice() override;

    bool open() override;
    void close() override;

    void play(int channel, std::shared_ptr<SoundAsset> asset, bool loop, float volume) override;
    void play_stream(int channel, std::shared_ptr<AudioStream> stream, bool loop, float volume) override;
    void stop(int channel) override;
    void set_volume(int channel, float volume) override;
    bool is_playing(int channel) const override;

  private:
    struct Channel {
        std::shared_ptr<SoundAsset> asset;
        std::shared_ptr<AudioStream> stream;
        size_t position = 0;
        float volume = 1.0f;
        bool loop = false;
        bool active = false;
    };

    static void data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count);
    void mix(float* output, int frames);

    ma_context context_{};
    bool context_open_ = false;
    ma_device device_{};
    bool device_open_ = false;
    mutable std::mutex mutex_;
    std::array<Channel, NUM_CHANNELS> channels_{};
};
