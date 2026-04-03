/**
 * @file CloudWatchSink.h
 * @brief Log sink that batches and ships log events to AWS CloudWatch Logs.
 *
 * Uses the PutLogEvents API with SigV4-signed requests.
 * Runs a background flush thread; thread-safe for concurrent log writes.
 */
#pragma once

#include "net/Http.h"
#include "utils/AwsSigV4.h"
#include "utils/Log.h"

#include <json.hpp>

#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class CloudWatchSink {
  public:
    struct Config {
        std::string region;
        std::string log_group;
        std::string log_stream;
        aws::Credentials credentials;
        int flush_interval_seconds = 5;
    };

    explicit CloudWatchSink(Config config)
        : config_(std::move(config)), host_("logs." + config_.region + ".amazonaws.com"), client_("https://" + host_) {
        client_.set_connection_timeout(5, 0);
        client_.set_read_timeout(10, 0);
    }

    ~CloudWatchSink() {
        stop();
    }

    /// Start the background flush thread.
    void start() {
        flush_thread_ = std::jthread([this](std::stop_token st) { flush_loop(st); });
    }

    /// Stop the flush thread and send any remaining buffered events.
    void stop() {
        if (flush_thread_.joinable()) {
            flush_thread_.request_stop();
            flush_thread_.join();
        }
        // Final flush
        flush();
    }

    /// Queue a log event. Called from any thread (the Log system holds its own mutex).
    void push(LogLevel level, const std::string& timestamp, const std::string& message) {
        auto now_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();

        std::string formatted = "[" + timestamp + "][" + log_level_name(level) + "] " + message;

        std::lock_guard lock(buffer_mutex_);
        buffer_.push_back({now_ms, std::move(formatted)});
    }

  private:
    struct BufferedEvent {
        int64_t timestamp_ms;
        std::string message;
    };

    void flush_loop(std::stop_token st) {
        while (!st.stop_requested()) {
            for (int i = 0; i < config_.flush_interval_seconds * 10 && !st.stop_requested(); ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            flush();
        }
    }

    void flush() {
        std::vector<BufferedEvent> events;
        {
            std::lock_guard lock(buffer_mutex_);
            if (buffer_.empty())
                return;
            events.swap(buffer_);
        }

        // CloudWatch requires events sorted by timestamp
        std::sort(events.begin(), events.end(),
                  [](const BufferedEvent& a, const BufferedEvent& b) { return a.timestamp_ms < b.timestamp_ms; });

        // CloudWatch PutLogEvents has a max of 10,000 events / 1MB per call.
        // Batch into chunks if needed.
        constexpr size_t MAX_EVENTS_PER_BATCH = 10000;
        for (size_t offset = 0; offset < events.size(); offset += MAX_EVENTS_PER_BATCH) {
            size_t end = std::min(offset + MAX_EVENTS_PER_BATCH, events.size());
            send_batch(events, offset, end);
        }
    }

    void send_batch(const std::vector<BufferedEvent>& events, size_t begin, size_t end) {
        // Build PutLogEvents JSON body
        nlohmann::json log_events = nlohmann::json::array();
        for (size_t i = begin; i < end; ++i) {
            log_events.push_back({
                {"timestamp", events[i].timestamp_ms},
                {"message", events[i].message},
            });
        }

        nlohmann::json body = {
            {"logGroupName", config_.log_group},
            {"logStreamName", config_.log_stream},
            {"logEvents", log_events},
        };

        std::string body_str = body.dump();

        // Sign the request
        aws::SignableRequest req;
        req.method = "POST";
        req.uri = "/";
        req.headers["host"] = host_;
        req.headers["content-type"] = "application/x-amz-json-1.1";
        req.headers["x-amz-target"] = "Logs_20140328.PutLogEvents";
        req.body = body_str;

        auto signed_headers = aws::sign(req, config_.credentials, config_.region, "logs");

        // Send via http::Client
        http::Headers headers = {
            {"Content-Type", "application/x-amz-json-1.1"},
            {"X-Amz-Target", "Logs_20140328.PutLogEvents"},
            {"X-Amz-Date", signed_headers.x_amz_date},
            {"X-Amz-Content-Sha256", signed_headers.x_amz_content_sha256},
            {"Authorization", signed_headers.authorization},
        };

        auto result = client_.Post("/", headers, body_str, "application/x-amz-json-1.1");
        if (!result || result->status != 200) {
            // Log to stderr directly — avoid re-entrant logging
            int status = result ? result->status : 0;
            std::string err_body = result ? result->body.substr(0, 500) : "connection failed";
            std::fprintf(stderr, "[CloudWatch] PutLogEvents failed: status=%d %s\n", status, err_body.c_str());
        }
    }

    Config config_;
    std::string host_;
    http::Client client_;

    std::mutex buffer_mutex_;
    std::vector<BufferedEvent> buffer_;

    std::jthread flush_thread_;
};
