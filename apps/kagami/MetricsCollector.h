#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

class GameRoom;
class RestRouter;
class ServerSettings;
class WebSocketServer;
class WsWorkerPool;

namespace http {
class Server;
}

/// Owns the metrics background thread and the cached /metrics text.
///
/// Registers all Prometheus gauges with MetricsRegistry, spawns a dedicated
/// thread that samples them every 2s (or on scrape request), and serves the
/// serialized text from an HTTP handler.
class MetricsCollector {
  public:
    MetricsCollector(GameRoom& room, RestRouter& router, const ServerSettings& cfg,
                     std::chrono::steady_clock::time_point start_time);
    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector(MetricsCollector&&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;
    MetricsCollector& operator=(MetricsCollector&&) = delete;

    /// Set after WebSocketServer is constructed (nullable until then).
    void set_ws(WebSocketServer* ws) {
        ws_ptr_.store(ws, std::memory_order_release);
    }

    /// Set before start() — not atomic because the happens-before from
    /// start() launching the thread provides the necessary synchronization.
    void set_ws_pool(WsWorkerPool* pool) {
        ws_pool_ = pool;
    }

    /// Register the /metrics HTTP handler and spawn the collection thread.
    void start(http::Server& http);

  private:
    struct MetricsTextCache {
        std::mutex mutex;
        std::condition_variable_any cv;
        std::shared_ptr<const std::string> text = std::make_shared<const std::string>();
        bool requested = false;

        void store(std::shared_ptr<const std::string> t) {
            std::lock_guard lock(mutex);
            text = std::move(t);
        }
        std::shared_ptr<const std::string> load() {
            std::lock_guard lock(mutex);
            return text;
        }
        void request_refresh() {
            {
                std::lock_guard lock(mutex);
                requested = true;
            }
            cv.notify_one();
        }
        bool wait_for_request(std::stop_token& st, std::chrono::steady_clock::duration timeout) {
            std::unique_lock lock(mutex);
            cv.wait_for(lock, st, timeout, [&] { return requested; });
            bool was_requested = requested;
            requested = false;
            return was_requested;
        }
    };

    std::shared_ptr<MetricsTextCache> cache_;
    std::jthread thread_;

    std::atomic<WebSocketServer*> ws_ptr_{nullptr};
    WsWorkerPool* ws_pool_{nullptr};

    GameRoom& room_;
    RestRouter& router_;
    const ServerSettings& cfg_;
    std::chrono::steady_clock::time_point start_time_;
};
