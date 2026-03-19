/**
 * @file Log.h
 * @brief Printf-style logging utility with severity levels.
 */
#pragma once

/**
 * @brief Severity levels for log output.
 *
 * Values are ordered by increasing severity. INVALID and COUNT are
 * sentinel values and should not be used for actual log messages.
 */
enum LogLevel {
    INVALID = 0, /**< Sentinel; not a valid log level. */
    VERBOSE,     /**< Verbose/trace-level detail. */
    DEBUG,       /**< Debug-level diagnostic information. */
    INFO,        /**< Informational messages. */
    WARNING,     /**< Warnings that may indicate a problem. */
    ERR,         /**< Errors that affect functionality. */
    FATAL,       /**< Fatal errors; the application cannot continue. */
    COUNT        /**< Sentinel; total number of log levels. */
};

/**
 * @brief Static logging utility.
 *
 * Provides a single printf-style method for emitting log messages
 * tagged with a severity level.
 */
class Log {
  public:
    /**
     * @brief Logs a formatted message at the given severity level.
     *
     * Uses printf-style formatting. The format string and variadic
     * arguments follow the same conventions as @c printf.
     *
     * @param log_level Severity level for this message.
     * @param fmt       Printf-style format string.
     * @param ...       Arguments matching the format specifiers in @p fmt.
     */
    static void log_print(LogLevel log_level, const char* fmt, ...);
};
