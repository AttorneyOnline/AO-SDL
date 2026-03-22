#pragma once

#include "event/Event.h"

#include <string>

/// Lightweight event published by the game thread to inform the UI of the
/// currently playing music track name.
class NowPlayingEvent : public Event {
  public:
    explicit NowPlayingEvent(std::string track) : track_(std::move(track)) {
    }

    std::string to_string() const override {
        return "NowPlaying: " + track_;
    }

    const std::string& track() const {
        return track_;
    }

  private:
    std::string track_;
};
