#pragma once

#include "event/Event.h"

/// Published when the server sends HP (health/penalty bar update).
/// side: 1 = defense, 2 = prosecution. value: 0-10.
class HealthBarEvent : public Event {
  public:
    HealthBarEvent(int side, int value) : side_(side), value_(value) {}

    std::string to_string() const override {
        return "HP side=" + std::to_string(side_) + " val=" + std::to_string(value_);
    }

    int side() const { return side_; }
    int value() const { return value_; }

  private:
    int side_;
    int value_;
};
