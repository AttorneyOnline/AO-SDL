#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread> // jthread
#include <vector>

class WebSocketServer;
class AOServer;
class RestRouter;
class GameRoom;
class ServerSettings;
namespace net {
class RateLimiter;
}

/// Lock-free atomic stats exposed for the metrics collector to read.
struct WsPollStats {
    std::atomic<uint64_t> idle_ns{0};
    std::atomic<uint64_t> busy_ns{0};
};

struct WsWorkerStats {
    std::atomic<uint64_t> idle_ns{0};
    std::atomic<uint64_t> busy_ns{0};
    std::atomic<int> active{0};
    std::atomic<uint64_t> frames_processed{0};
};

/// Manages the WebSocket worker thread pool and I/O poll thread.
///
/// Workers consume frames from per-client slot queues (hashed by client_id
/// for in-order processing). The poll thread drives I/O, dispatches incoming
/// frames to slots, flushes outbound sends, and runs periodic session expiry.
class WsWorkerPool {
  public:
    WsWorkerPool(WebSocketServer& ws, AOServer& ao_backend, RestRouter& router, GameRoom& room,
                 const ServerSettings& cfg, net::RateLimiter* rate_limiter = nullptr);

    /// Spawn worker threads and the poll thread. Wires ao_backend.set_send_func().
    void start();

    const WsPollStats& poll_stats() const {
        return poll_stats_;
    }
    const WsWorkerStats& worker_stats() const {
        return worker_stats_;
    }

    /// Total frames queued across all slots (acquires per-slot locks).
    size_t total_queued() const;

  private:
    struct WsWorkItem {
        uint64_t client_id;
        std::string data;
    };
    struct WsSlot {
        std::mutex mutex;
        std::condition_variable_any cv;
        std::deque<WsWorkItem> queue;
    };

    int slot_count_;
    std::vector<std::unique_ptr<WsSlot>> slots_;
    std::vector<std::jthread> workers_;
    std::jthread poll_thread_;

    WsPollStats poll_stats_;
    WsWorkerStats worker_stats_;

    WebSocketServer& ws_;
    AOServer& ao_;
    RestRouter& router_;
    GameRoom& room_;
    const ServerSettings& cfg_;
    net::RateLimiter* rate_limiter_;
};
