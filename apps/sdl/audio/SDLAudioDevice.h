#pragma once

#include "audio/IAudioDevice.h"

#include <SDL2/SDL.h>

#include <array>
#include <mutex>

/**
 * @brief SDL2-based audio playback device.
 *
 * Uses SDL_OpenAudioDevice with a pull-model callback that mixes up to 16
 * channels of decoded float32 PCM audio. The callback runs on SDL's audio
 * thread; channel state is protected by a mutex with a very short critical
 * section.
 */
class SDLAudioDevice : public IAudioDevice {
  public:
    static constexpr int NUM_CHANNELS = 16;
    static constexpr int SAMPLE_RATE = 48000;
    static constexpr int BUFFER_SAMPLES = 4096;

    SDLAudioDevice() = default;
    ~SDLAudioDevice() override;

    bool open() override;
    void close() override;

    struct ChannelInfo {
        int id;
        bool active;
        bool is_stream;
        bool stream_ready;
        bool stream_finished;
        bool stream_looping;
        int64_t loop_start;
        int64_t loop_end;
        float volume;
        size_t ring_available; // PCM samples in ring buffer (streams only)
    };
    std::vector<ChannelInfo> channel_snapshot() const;

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

    static void audio_callback(void* userdata, Uint8* stream, int len);
    void mix(float* output, int frames);

    SDL_AudioDeviceID device_id_ = 0;
    mutable std::mutex mutex_;
    std::array<Channel, NUM_CHANNELS> channels_{};
    float mix_buf_[BUFFER_SAMPLES * 2]{}; // scratch buffer for stream reads (stereo)
};
