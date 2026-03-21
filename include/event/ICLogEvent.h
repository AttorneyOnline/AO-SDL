#pragma once

#include "event/Event.h"

#include <string>

/// Lightweight event published when an IC message begins playing.
/// Used by the IC chat log widget to display message history.
class ICLogEvent : public Event {
  public:
    ICLogEvent(std::string showname, std::string message, int color_idx)
        : showname(std::move(showname)), message(std::move(message)), color_idx(color_idx) {}

    std::string to_string() const override {
        return showname + ": " + message;
    }

    const std::string& get_showname() const { return showname; }
    const std::string& get_message() const { return message; }
    int get_color_idx() const { return color_idx; }

  private:
    std::string showname;
    std::string message;
    int color_idx;
};
