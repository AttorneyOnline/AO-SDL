/**
 * @file ModerationAuditLog.h
 * @brief Dedicated audit trail for content moderation decisions.
 *
 * The moderation audit log is intentionally separate from the regular
 * Log system. Moderation events have higher sensitivity than ordinary
 * logs — you may want them going to a locked-down sink, to a sqlite
 * table for ad-hoc query, or both. You almost certainly don't want
 * them mixed into the firehose of WebSocket frame logs.
 *
 * ModerationAuditLog provides a fan-out interface: register one or
 * more named sinks, call record() from anywhere, and each sink
 * receives a structured ModerationEvent. No string serialization
 * happens until the sink decides how to format — the same event goes
 * to sqlite as a row, to a file sink as a JSON line, to Loki as a
 * push frame, etc.
 *
 * Thread safety: record() is safe to call from any thread. Sinks may
 * be added or removed at any time. Sinks must themselves be thread-safe
 * (callers hold the fan-out lock only briefly while copying the sink
 * list).
 */
#pragma once

#include "moderation/ModerationTrace.h"
#include "moderation/ModerationTypes.h"

#include <json.hpp>

#include <functional>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace moderation {

class ModerationAuditLog {
  public:
    using Sink = std::function<void(const ModerationEvent&)>;

    ModerationAuditLog() = default;
    ~ModerationAuditLog() = default;

    ModerationAuditLog(const ModerationAuditLog&) = delete;
    ModerationAuditLog& operator=(const ModerationAuditLog&) = delete;

    /// Set the minimum-action filter. Events with action severity
    /// below this threshold are dropped before any sink is called.
    void set_min_action(ModerationAction min);

    /// Add a named sink. Replaces any existing sink with the same name.
    /// The sink is called synchronously from record(); long-running or
    /// blocking sinks should fan out to their own thread internally.
    void add_sink(const std::string& name, Sink sink);

    /// Remove a named sink.
    void remove_sink(const std::string& name);

    /// Remove all sinks.
    void clear();

    /// Record an event. Fans out to every sink whose minimum action
    /// threshold is met. Safe to call from any thread.
    void record(const ModerationEvent& event);

    /// Query the list of currently-registered sink names. For diagnostics.
    std::vector<std::string> sink_names() const;

    /// Serialize a ModerationEvent to a nlohmann::json object. Defined
    /// here so sinks in different translation units can use it without
    /// each reimplementing the format.
    static void to_json(const ModerationEvent& event, nlohmann::json& j);

    /// Serialize to a compact JSON line (no trailing newline). Handy
    /// for file and stdout sinks.
    static std::string to_json_line(const ModerationEvent& event);

  private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, Sink> sinks_;
    ModerationAction min_action_ = ModerationAction::LOG;
};

/// Built-in sink: write events as JSON lines to an open std::ostream.
/// The factory captures a shared_ptr to the stream so it survives as
/// long as any sink is registered.
ModerationAuditLog::Sink make_stream_sink(std::shared_ptr<std::ostream> stream);

/// Built-in sink: append events as JSON lines to a file. Opens the
/// file in append mode; if it cannot be opened, returns a null sink.
ModerationAuditLog::Sink make_file_sink(const std::string& path);

/// Built-in sink: write events to stderr (not stdout, to avoid
/// clobbering REPL output) as JSON lines.
ModerationAuditLog::Sink make_stderr_sink();

// ---------------------------------------------------------------------------
// ModerationTraceLog — parallel fan-out for per-message traces
// ---------------------------------------------------------------------------
//
// ModerationAuditLog carries the "what was enforced" record (mute / kick
// / etc.) and is gated on a min_action threshold. ModerationTraceLog
// carries the "here's every layer's contribution to every check()"
// record with NO action gate — every check emits a trace, including
// the clean-traffic majority.
//
// The two are deliberately separate because their consumers are
// different:
//   - audit log  → SQLite, security-sensitive CloudWatch, enforcement
//                  review, always low volume.
//   - trace log  → Loki + JSONL file, dashboard/histogram analysis,
//                  always high volume.
//
// Sharing one class would require every audit sink to also handle the
// high-volume trace stream, which is wrong for CloudWatch ($) and
// SQLite (write amplification).

class ModerationTraceLog {
  public:
    using Sink = std::function<void(const ModerationTrace&)>;

    ModerationTraceLog() = default;
    ~ModerationTraceLog() = default;

    ModerationTraceLog(const ModerationTraceLog&) = delete;
    ModerationTraceLog& operator=(const ModerationTraceLog&) = delete;

    /// Enable/disable trace emission. When disabled, record() returns
    /// immediately without invoking any sinks. Lets operators toggle
    /// telemetry at runtime without tearing down the sink
    /// infrastructure.
    void set_enabled(bool enabled);

    /// Add a named sink. Replaces any existing sink with the same name.
    /// Called synchronously from record(); slow sinks should fan out
    /// to their own thread internally (the Loki sink does this).
    void add_sink(const std::string& name, Sink sink);

    /// Remove a named sink.
    void remove_sink(const std::string& name);

    /// Remove all sinks.
    void clear();

    /// Record a trace. Fans out to every sink when enabled. Safe to
    /// call from any thread.
    void record(const ModerationTrace& trace);

    /// Query the list of currently-registered sink names. For diagnostics.
    std::vector<std::string> sink_names() const;

  private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, Sink> sinks_;
    bool enabled_ = false;
};

/// Built-in sink: append traces as JSON lines to a file. Opens the
/// file in append mode; if it cannot be opened, returns a null sink.
ModerationTraceLog::Sink make_trace_file_sink(const std::string& path);

} // namespace moderation
