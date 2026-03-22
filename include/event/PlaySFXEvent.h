#pragma once

#include "asset/SoundAsset.h"
#include "event/Event.h"

#include <memory>
#include <string>

/// Published to request SFX playback on the audio thread.
class PlaySFXEvent : public Event {
  public:
    PlaySFXEvent(std::shared_ptr<SoundAsset> asset, bool loop, float volume)
        : asset_(std::move(asset)), loop_(loop), volume_(volume) {
    }

    std::string to_string() const override {
        return "PlaySFX: " + (asset_ ? asset_->path() : "(null)");
    }

    const std::shared_ptr<SoundAsset>& asset() const {
        return asset_;
    }
    bool loop() const {
        return loop_;
    }
    float volume() const {
        return volume_;
    }

  private:
    std::shared_ptr<SoundAsset> asset_;
    bool loop_;
    float volume_;
};
