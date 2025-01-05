#pragma once

enum LogLevel { INVALID = 0, VERBOSE, DEBUG, INFO, WARNING, ERR, FATAL, COUNT };

class Log {
  public:
    static void log_print(LogLevel log_level, const char* fmt, ...);
};
