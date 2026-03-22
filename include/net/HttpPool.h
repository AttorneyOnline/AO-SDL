#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/// Result of an HTTP request.
struct HttpResponse {
    int status = 0;    ///< HTTP status code (0 = connection error).
    std::string body;  ///< Response body.
    std::string error; ///< Error description (empty on success).
};

/// Callback invoked on the submitting thread's event loop (not the worker thread).
using HttpCallback = std::function<void(HttpResponse)>;

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
        HttpPriority priority;
    };

    struct CompletedRequest {
        HttpResponse response;
        HttpCallback callback;
    };

    void worker_loop();

    std::vector<std::thread> workers_;
    std::atomic<bool> running_{true};
    std::atomic<int> pending_{0};

    // Work queue sorted by priority (highest first)
    std::deque<Request> work_queue_;
    std::mutex work_mutex_;
    std::condition_variable work_cv_;

    // Result queue (completed, awaiting poll)
    std::deque<CompletedRequest> result_queue_;
    std::mutex result_mutex_;
};
