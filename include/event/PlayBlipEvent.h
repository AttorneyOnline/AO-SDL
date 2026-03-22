#pragma once

#include "asset/SoundAsset.h"
#include "event/Event.h"

#include <memory>
#include <string>

/// Published to request blip playback on the audio thread.
/// Routed to dedicated blip channels (round-robin) with blip volume.
class PlayBlipEvent : public Event {
  public:
    PlayBlipEvent(std::shared_ptr<SoundAsset> asset, float volume) : asset_(std::move(asset)), volume_(volume) {
    }

    std::string to_string() const override {
        return "PlayBlip: " + (asset_ ? asset_->path() : "(null)");
    }

    const std::shared_ptr<SoundAsset>& asset() const {
        return asset_;
    }
    float volume() const {
        return volume_;
    }

  private:
    std::shared_ptr<SoundAsset> asset_;
    float volume_;
};
