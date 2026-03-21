#pragma once

#include "event/Event.h"

#include <string>

/// Published when the server sends player count info (PN packet).
class PlayerCountEvent : public Event {
  public:
    PlayerCountEvent(int current, int max, std::string description)
        : current(current), max(max), description(std::move(description)) {}

    std::string to_string() const override {
        return std::to_string(current) + "/" + std::to_string(max);
    }

    int get_current() const { return current; }
    int get_max() const { return max; }
    const std::string& get_description() const { return description; }

  private:
    int current;
    int max;
    std::string description;
};
