#pragma once

#include "event/Event.h"

#include <string>
#include <vector>

/// Published when the server sends SM (combined), FA (areas only), or FM (music only).
/// When partial is true, only the non-empty list should be replaced.
class MusicListEvent : public Event {
  public:
    MusicListEvent(std::vector<std::string> areas, std::vector<std::string> tracks, bool partial = false)
        : areas_(std::move(areas)), tracks_(std::move(tracks)), partial_(partial) {
    }

    std::string to_string() const override {
        return std::to_string(areas_.size()) + " areas, " + std::to_string(tracks_.size()) + " tracks";
    }

    const std::vector<std::string>& areas() const {
        return areas_;
    }
    const std::vector<std::string>& tracks() const {
        return tracks_;
    }
    bool partial() const {
        return partial_;
    }

  private:
    std::vector<std::string> areas_;
    std::vector<std::string> tracks_;
    bool partial_ = false;
};
