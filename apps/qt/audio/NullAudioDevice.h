#pragma once

#include "audio/IAudioDevice.h"

/// No-op IAudioDevice stub used until a native Qt audio backend is implemented.
/// AudioThread requires a concrete device; this satisfies the interface while
/// producing no sound output.
class NullAudioDevice final : public IAudioDevice {
  public:
    bool open() override { return true; }
    void close() override {}
    void play(int, std::shared_ptr<SoundAsset>, bool, float) override {}
    void stop(int) override {}
    void set_volume(int, float) override {}
    void play_stream(int, std::shared_ptr<AudioStream>, bool, float) override {}
    bool is_playing(int) const override { return false; }
};
