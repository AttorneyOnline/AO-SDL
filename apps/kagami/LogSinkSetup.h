#pragma once

#include <fstream>
#include <memory>

class CloudWatchSink;
class LokiSink;
class ServerSettings;
class TerminalUI;

/// Manages the lifecycle of all log sinks (file, CloudWatch, Loki).
/// Call init() at startup and teardown() before destroying protocol backends.
class LogSinkSetup {
  public:
    LogSinkSetup();
    ~LogSinkSetup();

    void init(const ServerSettings& cfg, TerminalUI& ui, bool interactive);

    /// Remove all sinks and stop background flushers.
    /// Must be called before protocol backends are destroyed.
    void teardown();

  private:
    std::shared_ptr<std::ofstream> log_file_;
    std::unique_ptr<CloudWatchSink> cw_sink_;
    std::unique_ptr<LokiSink> loki_sink_;
};
