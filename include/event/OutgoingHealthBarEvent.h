#pragma once

#include "event/Event.h"

/// Published by the judge UI to request HP bar changes.
class OutgoingHealthBarEvent : public Event {
  public:
    OutgoingHealthBarEvent(int side, int value) : side_(side), value_(value) {
    }

    std::string to_string() const override {
        return "OutHP side=" + std::to_string(side_) + " val=" + std::to_string(value_);
    }

    int side() const {
        return side_;
    }
    int value() const {
        return value_;
    }

  private:
    int side_;
    int value_;
};
