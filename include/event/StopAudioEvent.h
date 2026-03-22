#pragma once

#include "event/Event.h"

#include <string>

/// Published to stop audio playback on a specific channel.
class StopAudioEvent : public Event {
  public:
    enum class Type { MUSIC, SFX, BLIP };

    StopAudioEvent(int channel, Type type) : channel_(channel), type_(type) {
    }

    std::string to_string() const override {
        return "StopAudio ch" + std::to_string(channel_);
    }

    int channel() const {
        return channel_;
    }
    Type type() const {
        return type_;
    }

  private:
    int channel_;
    Type type_;
};
