#include "utils/Log.h"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <mutex>

static const char* LOG_STRINGS[] = {"INVALID", "VERBOSE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};

const char* log_level_name(LogLevel level) {
    if (level >= LogLevel::COUNT || level <= LogLevel::INVALID)
        return "UNKNOWN";
    return LOG_STRINGS[level];
}

static std::mutex sink_mutex;
static Log::Sink log_sink;

void Log::set_sink(Sink sink) {
    std::lock_guard lock(sink_mutex);
    log_sink = std::move(sink);
}

void Log::log_print(LogLevel log_level, const char* fmt, ...) {
    if (log_level >= LogLevel::COUNT || log_level <= LogLevel::INVALID) {
        fprintf(stderr, "Logger error: invalid log level %d\n", log_level);
        return;
    }

    char timebuf[16];
    char messagebuf[4096];

    time_t timer = time(NULL);
#ifdef _WIN32
    struct tm tm_storage;
    localtime_s(&tm_storage, &timer);
    struct tm* tm_info = &tm_storage;
#else
    struct tm* tm_info = localtime(&timer);
#endif

    strftime(timebuf, sizeof(timebuf), "%I:%M:%S %p", tm_info);

    va_list args;
    va_start(args, fmt);
    vsnprintf(messagebuf, sizeof(messagebuf), fmt, args);
    va_end(args);

    printf("[%s][%s] %s\n", timebuf, LOG_STRINGS[log_level], messagebuf);

    // Fire callback sink (thread-safe)
    {
        std::lock_guard lock(sink_mutex);
        if (log_sink)
            log_sink(log_level, timebuf, messagebuf);
    }

    if (log_level >= LogLevel::FATAL) {
        exit(1);
    }
}
