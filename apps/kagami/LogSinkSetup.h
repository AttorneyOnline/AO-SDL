#pragma once

#include <fstream>
#include <memory>

class CloudWatchSink;
class LokiSink;
class ServerSettings;
class TerminalUI;

namespace aws {
class ImdsCredentialProvider;
}

namespace moderation {
class ModerationAuditLog;
class ModerationTraceLog;
} // namespace moderation

/// Manages the lifecycle of all log sinks (file, CloudWatch, Loki)
/// AND the dedicated moderation audit sinks. Keeping them in one
/// place means shutdown ordering is always correct: audit flushers
/// stop before the worker threads that may push to them.
///
/// Call init() at startup and teardown() before destroying protocol
/// backends. Passing a non-null ModerationAuditLog* enables the
/// dedicated audit channel — the Loki and CloudWatch instances here
/// are SEPARATE from the regular log sinks, with distinct stream
/// labels so moderation events can be queried without dragging the
/// full application log along with them.
class LogSinkSetup {
  public:
    LogSinkSetup();
    ~LogSinkSetup();

    void init(const ServerSettings& cfg, TerminalUI& ui, bool interactive,
              moderation::ModerationAuditLog* audit = nullptr, moderation::ModerationTraceLog* trace = nullptr);

    /// Remove all sinks and stop background flushers.
    /// Must be called before protocol backends are destroyed.
    void teardown();

  private:
    std::shared_ptr<std::ofstream> log_file_;
    std::unique_ptr<CloudWatchSink> cw_sink_;
    std::unique_ptr<LokiSink> loki_sink_;

    // Dedicated moderation audit sinks. Each owns its own Loki/CW
    // instance so it can push to a distinct stream label without
    // interfering with the application log sinks above.
    std::unique_ptr<LokiSink> mod_audit_loki_;
    std::unique_ptr<CloudWatchSink> mod_audit_cw_;
    moderation::ModerationAuditLog* audit_ = nullptr;

    // Dedicated per-message telemetry sinks. Parallel stream to the
    // audit log, different stream label. Only the Loki push client
    // is owned here; the file sink closes its own fstream when the
    // sink lambda is destroyed.
    std::unique_ptr<LokiSink> mod_trace_loki_;
    moderation::ModerationTraceLog* trace_ = nullptr;

    // Shared IMDS credential provider. Lazily constructed when the
    // first CloudWatch sink needs zero-config credentials. Both the
    // main and moderation audit sinks capture the same shared_ptr
    // so the IMDS cache is hit once per ~6h rotation regardless of
    // how many sinks are running.
    std::shared_ptr<aws::ImdsCredentialProvider> imds_provider_;
};
