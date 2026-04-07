#include "LogSinkSetup.h"
#include "CloudWatchSink.h"
#include "LokiSink.h"
#include "ServerSettings.h"
#include "TerminalUI.h"

#include "utils/Log.h"

LogSinkSetup::LogSinkSetup() = default;
LogSinkSetup::~LogSinkSetup() = default;

void LogSinkSetup::init(const ServerSettings& cfg, TerminalUI& ui, bool interactive) {
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
}
