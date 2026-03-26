#pragma once

#include "utils/Log.h"

#include <mutex>
#include <string>

/// Simple split-pane terminal UI using ANSI escape codes.
///
/// Layout:
///   rows 1..H-2   scrolling log area
///   row  H-1      horizontal rule
///   row  H        input prompt
///
/// Log output is routed here via Log::set_sink(). The prompt is redrawn
/// after each log line so it stays at the bottom.
class TerminalUI {
  public:
    ~TerminalUI();

    /// Set up the terminal: clear screen, draw separator, show prompt.
    /// Installs a SIGWINCH handler for terminal resize.
    void init();

    /// Restore terminal state (scroll region, cursor).
    void cleanup();

    /// Print a color-coded log line into the scrolling area. Thread-safe.
    void log(LogLevel level, const std::string& timestamp, const std::string& message);

    /// Print REPL command output into the scrolling area. Thread-safe.
    void print(const std::string& line);

    /// Redraw the prompt (call after reading a line of input).
    void show_prompt();

    /// Re-query terminal size and redraw the separator/prompt.
    /// Called automatically on SIGWINCH.
    void handle_resize();

  private:
    void emit(const std::string& line);
    void draw_separator();
    void apply_layout();

    int height_ = 24;
    int width_ = 80;
    std::mutex mutex_;
};
