#pragma once

#include "event/Event.h"

#include <string>

/// Published when the server broadcasts a music/area change (MC packet).
class MusicChangeEvent : public Event {
  public:
    MusicChangeEvent(std::string track, int char_id, std::string showname, int looping = -1, int channel = 0,
                     int effect_flags = 0)
        : track_(std::move(track)), char_id_(char_id), showname_(std::move(showname)), looping_(looping),
          channel_(channel), effect_flags_(effect_flags) {
    }

    std::string to_string() const override {
        return showname_ + " played " + track_;
    }

    const std::string& track() const {
        return track_;
    }
    int char_id() const {
        return char_id_;
    }
    const std::string& showname() const {
        return showname_;
    }
    int looping() const {
        return looping_;
    }
    int channel() const {
        return channel_;
    }
    int effect_flags() const {
        return effect_flags_;
    }

  private:
    std::string track_;
    int char_id_;
    std::string showname_;
    int looping_;
    int channel_;
    int effect_flags_;
};
