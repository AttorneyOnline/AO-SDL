/**
 * @file LokiSink.h
 * @brief Log sink that pushes batched log entries to Grafana Loki.
 *
 * Uses the Loki push API (POST /loki/api/v1/push) with JSON encoding.
 * Logs are labeled with {job="kagami", level="INFO"} for Grafana queries.
 */
#pragma once

#include "net/Http.h"
#include "utils/Log.h"

#include <json.hpp>

#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class LokiSink {
  public:
    struct Config {
        std::string url = "http://loki:3100"; // Loki base URL
        int flush_interval_seconds = 2;
        /// Grafana "job" label applied to every pushed stream. The
        /// default groups logs under `{job="kagami"}`. The moderation
        /// audit sink overrides this to `{job="kagami_mod_audit"}` so
        /// Grafana queries can separate the two streams cleanly.
        std::string job_label = "kagami";
    };

    explicit LokiSink(Config config) : config_(std::move(config)), client_(config_.url) {
        client_.set_connection_timeout(5, 0);
        client_.set_read_timeout(10, 0);
    }

    ~LokiSink() {
        stop();
    }

    void start() {
        flush_thread_ = std::jthread([this](std::stop_token st) { flush_loop(st); });
    }

    void stop() {
        if (flush_thread_.joinable()) {
            flush_thread_.request_stop();
            flush_thread_.join();
        }
        flush();
    }

    /// Queue a log event. Called from Log's sink callback.
    void push(LogLevel level, const std::string& timestamp, const std::string& message) {
        auto now_ns = std::to_string(
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count());

        // Omit [LEVEL] prefix — Grafana renders it as a colored badge from the label
        std::string formatted = "[" + timestamp + "] " + message;

        // Map to Grafana-recognized level names for colored badges
        const char* loki_level = "unknown";
        switch (level) {
        case VERBOSE:
        case DEBUG:
            loki_level = "debug";
            break;
        case INFO:
            loki_level = "info";
            break;
        case WARNING:
            loki_level = "warn";
            break;
        case ERR:
            loki_level = "error";
            break;
        case FATAL:
            loki_level = "critical";
            break;
        default:
            break;
        }

        std::lock_guard lock(buffer_mutex_);
        buffer_.push_back({loki_level, std::move(now_ns), std::move(formatted)});
    }

  private:
    struct BufferedEntry {
        std::string level;
        std::string timestamp_ns;
        std::string line;
    };

    void flush_loop(std::stop_token st) {
        while (!st.stop_requested()) {
            for (int i = 0; i < config_.flush_interval_seconds * 10 && !st.stop_requested(); ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            flush();
        }
    }

    void flush() {
        std::vector<BufferedEntry> entries;
        {
            std::lock_guard lock(buffer_mutex_);
            if (buffer_.empty())
                return;
            entries.swap(buffer_);
        }

        // Group entries by level label (Loki wants one stream per label set)
        std::unordered_map<std::string, nlohmann::json> streams;
        for (auto& e : entries) {
            auto& values = streams[e.level];
            if (values.is_null())
                values = nlohmann::json::array();
            values.push_back({e.timestamp_ns, e.line});
        }

        // Build Loki push payload
        nlohmann::json payload;
        payload["streams"] = nlohmann::json::array();
        for (auto& [level, values] : streams) {
            payload["streams"].push_back({
                {"stream", {{"job", config_.job_label}, {"level", level}}},
                {"values", values},
            });
        }

        auto body = payload.dump();
        auto result = client_.Post("/loki/api/v1/push", body, "application/json");
        if (!result) {
            std::fprintf(stderr, "[Loki] push failed: connection error (host=%s)\n", config_.url.c_str());
        }
        else if (result->status != 204 && result->status != 200) {
            std::fprintf(stderr, "[Loki] push failed: status=%d body=%s\n", result->status,
                         result->body.substr(0, 200).c_str());
        }
    }

    Config config_;
    http::Client client_;
    std::mutex buffer_mutex_;
    std::vector<BufferedEntry> buffer_;
    std::jthread flush_thread_;
};
