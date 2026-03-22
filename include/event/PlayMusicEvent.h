#pragma once

#include "asset/SoundAsset.h"
#include "event/Event.h"

#include <memory>
#include <string>

/// Published to request music playback on the audio thread.
class PlayMusicEvent : public Event {
  public:
    PlayMusicEvent(std::shared_ptr<SoundAsset> asset, int channel, bool loop, float volume, int effect_flags = 0)
        : asset_(std::move(asset)), channel_(channel), loop_(loop), volume_(volume), effect_flags_(effect_flags) {
    }

    std::string to_string() const override {
        return "PlayMusic ch" + std::to_string(channel_) + ": " + (asset_ ? asset_->path() : "(null)");
    }

    const std::shared_ptr<SoundAsset>& asset() const {
        return asset_;
    }
    int channel() const {
        return channel_;
    }
    bool loop() const {
        return loop_;
    }
    float volume() const {
        return volume_;
    }
    int effect_flags() const {
        return effect_flags_;
    }

  private:
    std::shared_ptr<SoundAsset> asset_;
    int channel_;
    bool loop_;
    float volume_;
    int effect_flags_;
};
