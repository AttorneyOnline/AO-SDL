#pragma once

#include "event/Event.h"

#include <string>
#include <vector>

/// Published when the server sends the combined area + music list (SM packet).
class MusicListEvent : public Event {
  public:
    MusicListEvent(std::vector<std::string> areas, std::vector<std::string> tracks)
        : areas_(std::move(areas)), tracks_(std::move(tracks)) {}

    std::string to_string() const override {
        return std::to_string(areas_.size()) + " areas, " + std::to_string(tracks_.size()) + " tracks";
    }

    const std::vector<std::string>& areas() const { return areas_; }
    const std::vector<std::string>& tracks() const { return tracks_; }

  private:
    std::vector<std::string> areas_;
    std::vector<std::string> tracks_;
};
