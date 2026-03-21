#pragma once

#include "event/Event.h"

#include <cstdint>

/// Published when the server sends TI (timer control).
/// action: 0 = start/sync, 1 = pause, 2 = show, 3 = hide.
class TimerEvent : public Event {
  public:
    TimerEvent(int timer_id, int action, int64_t time_ms = 0)
        : timer_id_(timer_id), action_(action), time_ms_(time_ms) {}

    std::string to_string() const override {
        return "TI id=" + std::to_string(timer_id_) + " action=" + std::to_string(action_);
    }

    int timer_id() const { return timer_id_; }
    int action() const { return action_; }
    int64_t time_ms() const { return time_ms_; }

  private:
    int timer_id_;
    int action_;
    int64_t time_ms_;
};
