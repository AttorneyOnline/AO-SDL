#pragma once

#include "asset/SoundAsset.h"
#include "audio/AudioStream.h"

#include <memory>

/**
 * @brief Abstract audio playback device interface.
 *
 * Backend-agnostic — the engine uses this interface to play sounds without
 * knowing whether SDL, CoreAudio, or anything else is underneath.
 */
class IAudioDevice {
  public:
    virtual ~IAudioDevice() = default;

    /// Open the audio device. Returns false on failure.
    virtual bool open() = 0;

    /// Close the audio device and release resources.
    virtual void close() = 0;

    /// Play a decoded sound on a logical channel.
    /// @param channel Logical channel index (0-15).
    /// @param asset   Decoded audio data (shared ownership kept while playing).
    /// @param loop    Whether to loop the sound.
    /// @param volume  Per-channel volume (0.0 - 1.0).
    virtual void play(int channel, std::shared_ptr<SoundAsset> asset, bool loop, float volume) = 0;

    /// Stop playback on a channel.
    virtual void stop(int channel) = 0;

    /// Set volume for a channel (0.0 - 1.0).
    virtual void set_volume(int channel, float volume) = 0;

    /// Play a streaming audio source on a logical channel.
    virtual void play_stream(int channel, std::shared_ptr<AudioStream> stream, bool loop, float volume) = 0;

    /// Whether a channel is currently playing.
    virtual bool is_playing(int channel) const = 0;
};
