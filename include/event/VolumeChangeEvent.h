#pragma once

#include "event/Event.h"

#include <string>

/// Published when the user adjusts a volume slider in the UI.
class VolumeChangeEvent : public Event {
  public:
    enum class Category { MUSIC, SFX, BLIP, MASTER };

    VolumeChangeEvent(Category category, float volume) : category_(category), volume_(volume) {
    }

    std::string to_string() const override {
        return "VolumeChange: " + std::to_string(volume_);
    }

    Category category() const {
        return category_;
    }
    float volume() const {
        return volume_;
    }

  private:
    Category category_;
    float volume_;
};
