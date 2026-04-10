#include "LogSinkSetup.h"
#include "CloudWatchSink.h"
#include "LokiSink.h"
#include "ServerSettings.h"
#include "TerminalUI.h"

#include "moderation/ModerationAuditLog.h"
#include "utils/Log.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

LogSinkSetup::LogSinkSetup() = default;
LogSinkSetup::~LogSinkSetup() = default;

namespace {

/// Pick a LogLevel for a moderation action so that the reused Loki /
/// CloudWatch sinks can color/filter properly without needing to
/// understand moderation semantics. Anything kick/ban/perma-ban is
/// mapped to ERROR so oncall pages can be wired against log-level
/// alerts without parsing the JSON payload.
LogLevel severity_for(moderation::ModerationAction action) {
    using A = moderation::ModerationAction;
    switch (action) {
    case A::NONE:
    case A::LOG:
    case A::CENSOR:
        return INFO;
    case A::DROP:
    case A::MUTE:
        return WARNING;
    case A::KICK:
    case A::BAN:
    case A::PERMA_BAN:
        return ERR;
    }
    return INFO;
}

/// Format a system-clock timestamp_ms as "HH:MM:SS" for the sink
/// message prefix. Matches the existing Log system's timestamp shape.
std::string format_timestamp(int64_t ts_ms) {
    std::time_t tt = static_cast<std::time_t>(ts_ms / 1000);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

} // namespace

void LogSinkSetup::init(const ServerSettings& cfg, TerminalUI& ui, bool interactive,
                        moderation::ModerationAuditLog* audit) {
    audit_ = audit;
    Log::set_stdout_level(cfg.console_log_level());

    if (interactive) {
        ui.init();
        Log::set_sink([&ui](LogLevel level, const std::string& timestamp,
                            const std::string& message) { ui.log(level, timestamp, message); },
                      cfg.console_log_level());
    }

    // File log sink
    if (!cfg.log_file().empty()) {
        log_file_ = std::make_shared<std::ofstream>(cfg.log_file(), std::ios::app);
        if (log_file_->is_open()) {
            Log::add_sink(
                "file",
                [lf = log_file_](LogLevel level, const std::string& timestamp, const std::string& message) {
                    *lf << "[" << timestamp << "][" << log_level_name(level) << "] " << message << "\n";
                    lf->flush();
                },
                cfg.file_log_level());
        }
        else {
            Log::log_print(WARNING, "Could not open log file: %s", cfg.log_file().c_str());
            log_file_.reset();
        }
    }

    // CloudWatch log sink
    if (!cfg.cloudwatch_region().empty() && !cfg.cloudwatch_log_group().empty()) {
        CloudWatchSink::Config cw_cfg;
        cw_cfg.region = cfg.cloudwatch_region();
        cw_cfg.log_group = cfg.cloudwatch_log_group();
        cw_cfg.log_stream = cfg.cloudwatch_log_stream();
        cw_cfg.credentials.access_key_id = cfg.cloudwatch_access_key_id();
        cw_cfg.credentials.secret_access_key = cfg.cloudwatch_secret_access_key();
        cw_cfg.flush_interval_seconds = cfg.cloudwatch_flush_interval();

        if (cw_cfg.log_stream.empty())
            cw_cfg.log_stream = cfg.server_name();

        Log::log_print(INFO, "CloudWatch: group=%s stream=%s region=%s", cw_cfg.log_group.c_str(),
                       cw_cfg.log_stream.c_str(), cw_cfg.region.c_str());

        cw_sink_ = std::make_unique<CloudWatchSink>(std::move(cw_cfg));
        Log::add_sink(
            "cloudwatch",
            [&cw = *cw_sink_](LogLevel level, const std::string& timestamp, const std::string& message) {
                cw.push(level, timestamp, message);
            },
            cfg.cloudwatch_log_level());
        cw_sink_->start();
    }

    // Loki log sink
    if (!cfg.loki_url().empty()) {
        LokiSink::Config loki_cfg;
        loki_cfg.url = cfg.loki_url();
        loki_sink_ = std::make_unique<LokiSink>(std::move(loki_cfg));
        Log::add_sink(
            "loki",
            [&lk = *loki_sink_](LogLevel level, const std::string& timestamp, const std::string& message) {
                lk.push(level, timestamp, message);
            },
            cfg.console_log_level());
        loki_sink_->start();
        Log::log_print(INFO, "Loki: pushing to %s", cfg.loki_url().c_str());
    }

    // --- Dedicated moderation audit sinks ---
    //
    // These share the Loki / CloudWatch sink implementations but point
    // to distinct streams (via job_label / log_stream) so moderation
    // events are queryable independently of application logs. All are
    // opt-in: empty config strings mean the sink is not constructed.
    if (audit_) {
        auto cm_cfg = cfg.content_moderation_config();

        // Loki audit sink
        if (!cm_cfg.audit.loki_url.empty()) {
            LokiSink::Config lcfg;
            lcfg.url = cm_cfg.audit.loki_url;
            lcfg.job_label =
                cm_cfg.audit.loki_stream_label.empty() ? "kagami_mod_audit" : cm_cfg.audit.loki_stream_label;
            mod_audit_loki_ = std::make_unique<LokiSink>(std::move(lcfg));
            mod_audit_loki_->start();
            audit_->add_sink("loki", [lk = mod_audit_loki_.get()](const moderation::ModerationEvent& ev) {
                std::string json = moderation::ModerationAuditLog::to_json_line(ev);
                lk->push(severity_for(ev.action), format_timestamp(ev.timestamp_ms), json);
            });
            Log::log_print(INFO, "ModerationAuditLog: Loki sink -> %s (label=%s)",
                           cm_cfg.audit.loki_url.c_str(),
                           cm_cfg.audit.loki_stream_label.c_str());
        }

        // CloudWatch audit sink: reuses the main cloudwatch credentials
        // + region but with its own log_group / log_stream so audit
        // events don't mix with server logs.
        if (!cm_cfg.audit.cloudwatch_log_group.empty() && !cfg.cloudwatch_region().empty()) {
            CloudWatchSink::Config cwcfg;
            cwcfg.region = cfg.cloudwatch_region();
            cwcfg.log_group = cm_cfg.audit.cloudwatch_log_group;
            cwcfg.log_stream =
                cm_cfg.audit.cloudwatch_log_stream.empty() ? "moderation" : cm_cfg.audit.cloudwatch_log_stream;
            cwcfg.credentials.access_key_id = cfg.cloudwatch_access_key_id();
            cwcfg.credentials.secret_access_key = cfg.cloudwatch_secret_access_key();
            cwcfg.flush_interval_seconds = cfg.cloudwatch_flush_interval();
            mod_audit_cw_ = std::make_unique<CloudWatchSink>(std::move(cwcfg));
            mod_audit_cw_->start();
            audit_->add_sink("cloudwatch", [cw = mod_audit_cw_.get()](const moderation::ModerationEvent& ev) {
                std::string json = moderation::ModerationAuditLog::to_json_line(ev);
                cw->push(severity_for(ev.action), format_timestamp(ev.timestamp_ms), json);
            });
            Log::log_print(INFO, "ModerationAuditLog: CloudWatch sink -> %s/%s",
                           cm_cfg.audit.cloudwatch_log_group.c_str(),
                           cm_cfg.audit.cloudwatch_log_stream.c_str());
        }
    }
}

void LogSinkSetup::teardown() {
    Log::remove_sink("loki");
    Log::remove_sink("cloudwatch");
    Log::remove_sink("file");
    Log::set_sink(nullptr);
    if (loki_sink_)
        loki_sink_->stop();
    if (cw_sink_)
        cw_sink_->stop();

    // Moderation audit sinks. Clear the registrations first so no
    // new events reach them while flushers are stopping.
    if (audit_) {
        audit_->remove_sink("loki");
        audit_->remove_sink("cloudwatch");
    }
    if (mod_audit_loki_)
        mod_audit_loki_->stop();
    if (mod_audit_cw_)
        mod_audit_cw_->stop();
}
