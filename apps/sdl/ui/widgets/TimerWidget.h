#pragma once

#include "ui/IWidget.h"

#include <chrono>

/// Displays courtroom timers (TI packet). Supports multiple timers.
class TimerWidget : public IWidget {
  public:
    void handle_events() override;
    void render() override;

  private:
    static constexpr int MAX_TIMERS = 5;

    struct Timer {
        bool visible = false;
        bool running = false;
        int64_t remaining_ms = 0;
        std::chrono::steady_clock::time_point last_tick{};
    };

    Timer timers_[MAX_TIMERS]{};
};
