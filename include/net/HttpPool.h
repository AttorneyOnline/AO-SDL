#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

/// Result of an HTTP request.
struct HttpResponse {
    int status = 0;         ///< HTTP status code (0 = connection error).
    std::string body;       ///< Response body.
    std::string error;      ///< Error description (empty on success).
};

/// Callback invoked on the submitting thread's event loop (not the worker thread).
using HttpCallback = std::function<void(HttpResponse)>;

/// A simple thread pool for HTTP GET requests.
///
/// Submit requests with get(). The callback is not invoked directly on a worker
/// thread — instead, completed responses are queued and delivered when the owner
/// calls poll() on its own thread (typically the main/UI thread).
///
/// Example:
///   HttpPool pool(2);
///   pool.get("http://example.com", "/path", [](HttpResponse r) {
///       if (r.status == 200) process(r.body);
///   });
///   // In your frame loop:
///   pool.poll();
class HttpPool {
  public:
    /// Create a pool with the given number of worker threads.
    explicit HttpPool(int num_threads = 2);

    /// Signal all workers to stop and join them.
    ~HttpPool();

    HttpPool(const HttpPool&) = delete;
    HttpPool& operator=(const HttpPool&) = delete;

    /// Submit an HTTP GET request. The callback will be delivered via poll().
    /// @param host  Scheme + host, e.g. "http://example.com" or "https://example.com".
    /// @param path  Request path, e.g. "/servers".
    /// @param cb    Callback receiving the response.
    void get(const std::string& host, const std::string& path, HttpCallback cb);

    /// Deliver completed responses to their callbacks. Call this on the thread
    /// where you want callbacks to run (typically the main thread).
    /// Returns the number of callbacks delivered.
    int poll();

    /// Number of requests currently in-flight or queued.
    int pending() const { return pending_.load(std::memory_order_relaxed); }

  private:
    struct Request {
        std::string host;
        std::string path;
        HttpCallback callback;
    };

    struct CompletedRequest {
        HttpResponse response;
        HttpCallback callback;
    };

    void worker_loop();

    std::vector<std::thread> workers_;
    std::atomic<bool> running_{true};
    std::atomic<int> pending_{0};

    // Work queue (pending requests)
    std::queue<Request> work_queue_;
    std::mutex work_mutex_;
    std::condition_variable work_cv_;

    // Result queue (completed, awaiting poll)
    std::queue<CompletedRequest> result_queue_;
    std::mutex result_mutex_;
};
