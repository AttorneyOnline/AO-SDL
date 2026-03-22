#pragma once

#include "event/Event.h"

#include <string>

/// Published by the game thread to request music playback.
/// The audio thread handles downloading, decoding, and streaming.
class PlayMusicRequestEvent : public Event {
  public:
    PlayMusicRequestEvent(std::string path, int channel, bool loop, float volume)
        : path_(std::move(path)), channel_(channel), loop_(loop), volume_(volume) {
    }

    std::string to_string() const override {
        return "PlayMusicRequest: " + path_;
    }

    const std::string& path() const {
        return path_;
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

  private:
    std::string path_;
    int channel_;
    bool loop_;
    float volume_;
};
