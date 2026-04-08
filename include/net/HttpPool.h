#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Http2Connection;

/// Result of an HTTP request.
struct HttpResponse {
    int status = 0;    ///< HTTP status code (0 = connection error).
    std::string body;  ///< Response body.
    std::string error; ///< Error description (empty on success).
};

/// Callback invoked on the submitting thread's event loop (not the worker thread).
using HttpCallback = std::function<void(HttpResponse)>;

/// Callback invoked on a worker thread for each chunk of a streaming download.
/// Return false to cancel the download.
using HttpChunkCallback = std::function<bool(const uint8_t* data, size_t len)>;

/// Priority levels for HTTP requests (higher = more urgent).
enum class HttpPriority {
    LOW = 0,      ///< Background work: char icons, non-urgent prefetch.
    NORMAL = 1,   ///< Standard: emote/background prefetch for queued messages.
    HIGH = 2,     ///< Urgent: assets needed for the currently playing message.
    CRITICAL = 3, ///< Blocking: config files, extensions.json.
};

/// A thread pool for HTTP GET requests with priority scheduling.
///
/// Submit requests with get(). The callback is not invoked directly on a worker
/// thread — instead, completed responses are queued and delivered when the owner
/// calls poll() on its own thread (typically the main/UI thread).
///
/// Higher-priority requests are dequeued before lower ones.
class HttpPool {
  public:
    /// Create a pool with the given number of worker threads.
    explicit HttpPool(int num_threads = 2);

    /// Signal all workers to stop and join them.
    ~HttpPool();

    /// Stop all worker threads. Safe to call multiple times.
    void stop();

    HttpPool(const HttpPool&) = delete;
    HttpPool& operator=(const HttpPool&) = delete;

    /// Submit an HTTP GET request with a priority level.
    void get(const std::string& host, const std::string& path, HttpCallback cb,
             HttpPriority priority = HttpPriority::NORMAL);

    /// Submit a streaming HTTP GET. The on_chunk callback is invoked on a worker
    /// thread for each received chunk. The completion callback follows the normal
    /// path (delivered via poll()). Use for large downloads like music.
    void get_streaming(const std::string& host, const std::string& path, HttpChunkCallback on_chunk, HttpCallback cb,
                       HttpPriority priority = HttpPriority::NORMAL);

    /// Drop all queued (not yet started) requests at or below the given priority.
    /// In-flight requests are unaffected.
    void drop_below(HttpPriority threshold);

    /// Deliver completed responses to their callbacks. Call this on the thread
    /// where you want callbacks to run (typically the main thread).
    /// Returns the number of callbacks delivered.
    int poll();

    /// Number of requests currently in-flight or queued.
    int pending() const {
        return pending_.load(std::memory_order_relaxed);
    }

  private:
    struct Request {
        std::string host;
        std::string path;
        HttpCallback callback;
        HttpChunkCallback chunk_callback; ///< Non-null for streaming requests.
        HttpPriority priority;
    };

    struct CompletedRequest {
        HttpResponse response;
        HttpCallback callback;
    };

    static constexpr int NUM_PRIORITIES = 4;

    void worker_loop(std::stop_token st);
    bool pop_highest(Request& out);

    std::vector<std::jthread> workers_;
    std::atomic<int> pending_{0};

    // Per-priority work queues — index 0 = LOW, 3 = CRITICAL.
    // Avoids O(N) sorted insertion; enqueue is O(1), dequeue scans 4 buckets.
    std::deque<Request> work_queues_[NUM_PRIORITIES];
    std::mutex work_mutex_;
    std::condition_variable_any work_cv_;

    // Result queue (completed, awaiting poll)
    std::deque<CompletedRequest> result_queue_;
    std::mutex result_mutex_;

    // HTTP/2 connection pool: one multiplexed connection per host.
    // Workers try h2 first; if server doesn't support it, h2_failed_hosts_
    // prevents repeated ALPN attempts.
    std::unordered_map<std::string, std::shared_ptr<Http2Connection>> h2_connections_;
    std::unordered_set<std::string> h2_failed_hosts_;     // hosts that don't support h2
    std::unordered_set<std::string> h2_eligible_hosts_;   // hosts with at least one successful h1 request
    std::unordered_set<std::string> h2_connecting_hosts_; // hosts currently being connected (prevents races)
    std::mutex h2_mutex_;

    std::shared_ptr<Http2Connection> get_h2(const std::string& host);
    void mark_h2_eligible(const std::string& host);
};
