/**
 * @file Log.h
 * @brief Printf-style logging utility with severity levels and callback sink.
 */
#pragma once

#include <functional>
#include <string>

enum LogLevel {
    INVALID = 0,
    VERBOSE,
    DEBUG,
    INFO,
    WARNING,
    ERR,
    FATAL,
    COUNT
};

/// Returns the short string name for a log level (e.g. "DEBUG", "WARNING").
const char* log_level_name(LogLevel level);

class Log {
  public:
    /// Printf-style log message.
    static void log_print(LogLevel log_level, const char* fmt, ...);

    /// Callback type for log sinks. Receives level, timestamp, and formatted message.
    using Sink = std::function<void(LogLevel level, const std::string& timestamp, const std::string& message)>;

    /// Register an additional log sink. Called from any thread — must be thread-safe.
    /// Only one sink is supported (last one wins). Pass nullptr to clear.
    static void set_sink(Sink sink);
};
