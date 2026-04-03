/**
 * @file Log.h
 * @brief Modern C++20 logging utility with severity levels and event-based sinks.
 *
 * Log messages are delivered via EventChannel<LogEvent> so multiple consumers
 * (stdout, in-window overlay, file) can subscribe independently.
 */
#pragma once

#include <chrono>
#include <format>
#include <functional>
#include <string>
#include <string_view>

enum LogLevel { INVALID = 0, VERBOSE, DEBUG, INFO, WARNING, ERR, FATAL, COUNT };

/// Returns the short string name for a log level (e.g. "DEBUG", "WARNING").
constexpr const char* log_level_name(LogLevel level) {
    constexpr const char* names[] = {"INVALID", "VERBOSE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};
    if (level >= LogLevel::COUNT || level <= LogLevel::INVALID)
        return "UNKNOWN";
    return names[level];
}

/// A single log entry, published via EventChannel<LogEvent>.
struct LogEvent {
    LogLevel level;
    std::chrono::system_clock::time_point time;
    std::string message;

    /// Format the timestamp as "HH:MM:SS AM/PM".
    std::string timestamp() const;
};

class Log {
  public:
    /// Log a formatted message (C++20 std::format syntax).
    /// Example: Log::info("loaded {} bytes from {}", size, path);
    template <typename... Args>
    static void log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        log_impl(level, std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    static void verbose(std::format_string<Args...> fmt, Args&&... args) {
        log(VERBOSE, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void debug(std::format_string<Args...> fmt, Args&&... args) {
        log(DEBUG, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void info(std::format_string<Args...> fmt, Args&&... args) {
        log(INFO, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void warn(std::format_string<Args...> fmt, Args&&... args) {
        log(WARNING, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void error(std::format_string<Args...> fmt, Args&&... args) {
        log(ERR, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void fatal(std::format_string<Args...> fmt, Args&&... args) {
        log(FATAL, fmt, std::forward<Args>(args)...);
    }

    /// Legacy printf-style API (kept for compatibility during migration).
    static void log_print(LogLevel log_level, const char* fmt, ...);

    /// Callback type for log sinks. Receives level, timestamp, and formatted message.
    using Sink = std::function<void(LogLevel level, const std::string& timestamp, const std::string& message)>;

    /// Register an additional log sink. Called from any thread — must be thread-safe.
    /// Only one sink is supported (last one wins). Pass nullptr to clear.
    /// min_level filters: only events at or above this level reach the sink.
    static void set_sink(Sink sink, LogLevel min_level = VERBOSE);

    /// Add a named sink. Multiple sinks can coexist. Thread-safe.
    /// min_level filters: only events at or above this level reach the sink.
    static void add_sink(const std::string& name, Sink sink, LogLevel min_level = VERBOSE);

    /// Remove a named sink by name. Thread-safe.
    static void remove_sink(const std::string& name);

    /// Set the minimum level for stdout output. Default is VERBOSE (everything).
    static void set_stdout_level(LogLevel min_level);

  private:
    static void log_impl(LogLevel level, std::string message);
};
