#include "utils/Log.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#define TIMEBUF_SIZE 16
#define MESSAGEBUF_SIZE 4096

const char* LOG_STRINGS[] = {"INVALID", "VERBOSE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};

void Log::log_print(LogLevel log_level, const char* fmt, ...) {
    if (log_level >= LogLevel::COUNT || log_level <= LogLevel::INVALID) {
        fprintf(stderr, "Logger error: invalid log level %d\n", log_level);
        return;
    }

    char timebuf[TIMEBUF_SIZE];
    char messagebuf[MESSAGEBUF_SIZE];

    time_t timer = time(NULL);
    struct tm* tm_info = localtime(&timer); // TODO : MSVC considers this unsafe and use localtime_s instead.

    strftime(timebuf, TIMEBUF_SIZE, "%I:%M:%S %p", tm_info);

    va_list args;
    va_start(args, fmt);
    vsnprintf(messagebuf, MESSAGEBUF_SIZE, fmt, args);
    va_end(args);

    printf("[%s][%s] %s\n", timebuf, LOG_STRINGS[log_level], messagebuf);

    if (log_level >= LogLevel::FATAL) {
        exit(1);
    }
}
