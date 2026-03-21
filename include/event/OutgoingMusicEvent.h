#pragma once

#include "event/Event.h"

#include <string>

/// Published by the UI when the user selects a music track or area.
/// The network layer translates this to an MC packet.
class OutgoingMusicEvent : public Event {
  public:
    OutgoingMusicEvent(std::string name, std::string showname = "")
        : name_(std::move(name)), showname_(std::move(showname)) {}

    std::string to_string() const override {
        return "MC: " + name_;
    }

    const std::string& name() const { return name_; }
    const std::string& showname() const { return showname_; }

  private:
    std::string name_;
    std::string showname_;
};
