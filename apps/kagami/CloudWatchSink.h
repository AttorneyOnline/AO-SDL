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

        // Ensure strictly increasing timestamps so CloudWatch preserves insertion order.
        // Multiple logs within the same ms get successive timestamps.
        std::lock_guard lock(buffer_mutex_);
        if (now_ms <= last_timestamp_ms_)
            now_ms = last_timestamp_ms_ + 1;
        last_timestamp_ms_ = now_ms;

        std::string formatted = "[" + timestamp + "][" + log_level_name(level) + "] " + message;
        buffer_.push_back({now_ms, std::move(formatted)});
    }

  private:
    int64_t last_timestamp_ms_ = 0; ///< For monotonic timestamps within a batch.

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

        // Sign the request.
        // Note: do NOT include content-type in the signable headers — httplib's
        // Post() adds its own Content-Type from the content_type parameter, and
        // since Headers is a multimap, including it here too would produce a
        // duplicate header that breaks SigV4 verification.
        aws::SignableRequest req;
        req.method = "POST";
        req.uri = "/";
        req.headers["host"] = host_;
        req.headers["x-amz-target"] = "Logs_20140328.PutLogEvents";
        req.body = body_str;

        auto signed_headers = aws::sign(req, config_.credentials, config_.region, "logs");

        // Send via http::Client
        http::Headers headers = {
            {"X-Amz-Target", "Logs_20140328.PutLogEvents"},
            {"X-Amz-Date", signed_headers.x_amz_date},
            {"X-Amz-Content-Sha256", signed_headers.x_amz_content_sha256},
            {"Authorization", signed_headers.authorization},
        };

        auto result = client_.Post("/", headers, body_str, "application/x-amz-json-1.1");
        if (!result || result->status != 200) {
            int status = result ? result->status : 0;
            std::string err_body = result ? result->body.substr(0, 500) : "connection failed";

            // If the stream doesn't exist, try to create it and retry.
            // This happens on first-use of new log streams (e.g. the
            // moderation audit stream created at kagami boot) where
            // the group exists but the stream has never been written.
            //
            // Retry policy:
            //
            //   - ResourceNotFoundException → call CreateLogStream, then
            //     re-sign + retry PutLogEvents. Only mark stream_created_
            //     to true on actual SUCCESS so a transient network error
            //     during create doesn't lock us out forever. If the
            //     create fails, the next flush cycle will try again,
            //     with simple backoff via the create_attempts counter
            //     (caps at 10 attempts total before giving up).
            //
            //   - Any other 4xx/5xx → logged but not retried; the batch
            //     is dropped. CloudWatch Logs drops bad batches anyway
            //     (oversized, bad timestamps, etc) and we don't want to
            //     retry quota-limit failures indefinitely.
            if (result && result->status == 400 && err_body.find("ResourceNotFoundException") != std::string::npos &&
                !stream_created_ && create_attempts_ < 10) {
                ++create_attempts_;
                if (create_log_stream()) {
                    // Re-sign because SigV4 signatures are single-use
                    // and carry a timestamp.
                    auto retry_signed = aws::sign(req, config_.credentials, config_.region, "logs");
                    http::Headers retry_headers = {
                        {"X-Amz-Target", "Logs_20140328.PutLogEvents"},
                        {"X-Amz-Date", retry_signed.x_amz_date},
                        {"X-Amz-Content-Sha256", retry_signed.x_amz_content_sha256},
                        {"Authorization", retry_signed.authorization},
                    };
                    auto retry = client_.Post("/", retry_headers, body_str, "application/x-amz-json-1.1");
                    if (retry && retry->status == 200) {
                        // Only latch on success — transient failures
                        // leave stream_created_ false so the next
                        // batch can try again.
                        stream_created_ = true;
                        std::fprintf(stderr, "[CloudWatch] auto-created stream %s and recovered\n",
                                     config_.log_stream.c_str());
                        return;
                    }
                    status = retry ? retry->status : 0;
                    err_body = retry ? retry->body.substr(0, 500) : "connection failed";
                }
            }
            std::fprintf(stderr, "[CloudWatch] PutLogEvents failed: status=%d %s\n", status, err_body.c_str());
        }
    }

    /// Attempt to create our log stream via the CreateLogStream API.
    /// Returns true on 200 OK or if the stream already exists.
    bool create_log_stream() {
        nlohmann::json body = {
            {"logGroupName", config_.log_group},
            {"logStreamName", config_.log_stream},
        };
        std::string body_str = body.dump();

        aws::SignableRequest req;
        req.method = "POST";
        req.uri = "/";
        req.headers["host"] = host_;
        req.headers["x-amz-target"] = "Logs_20140328.CreateLogStream";
        req.body = body_str;

        auto signed_headers = aws::sign(req, config_.credentials, config_.region, "logs");
        http::Headers headers = {
            {"X-Amz-Target", "Logs_20140328.CreateLogStream"},
            {"X-Amz-Date", signed_headers.x_amz_date},
            {"X-Amz-Content-Sha256", signed_headers.x_amz_content_sha256},
            {"Authorization", signed_headers.authorization},
        };
        auto result = client_.Post("/", headers, body_str, "application/x-amz-json-1.1");
        if (!result) {
            std::fprintf(stderr, "[CloudWatch] CreateLogStream transport failure\n");
            return false;
        }
        if (result->status == 200)
            return true;
        // ResourceAlreadyExistsException is fine — someone else created it.
        if (result->body.find("ResourceAlreadyExistsException") != std::string::npos)
            return true;
        std::fprintf(stderr, "[CloudWatch] CreateLogStream failed: status=%d %s\n", result->status,
                     result->body.substr(0, 500).c_str());
        return false;
    }

    Config config_;
    std::string host_;
    http::Client client_;

    std::mutex buffer_mutex_;
    std::vector<BufferedEvent> buffer_;

    /// Latched to true only on a SUCCESSFUL CreateLogStream + retry
    /// round-trip. A transient failure during the create leaves this
    /// false so the next flush cycle can try again.
    bool stream_created_ = false;
    /// Hard cap on create attempts. Protects against a permanent
    /// IAM / quota / config failure turning into an API-hammering
    /// retry loop on every flush. Once we've tried 10 times and
    /// every attempt has failed, stop trying and let the events
    /// drop. 10 attempts × 5s flush interval = 50 seconds of retries
    /// before giving up, which is enough to absorb a short CloudWatch
    /// blip but not a persistent misconfiguration.
    int create_attempts_ = 0;

    std::jthread flush_thread_;
};
