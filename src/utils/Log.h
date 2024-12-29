#ifndef UTILS_LOG_H
#define UTILS_LOG_H

enum LogLevel { INVALID = 0, VERBOSE, DEBUG, INFO, WARNING, ERROR, FATAL, COUNT };

class Log {
  public:
    static void log_print(LogLevel log_level, const char* fmt, ...);
};

#endif
