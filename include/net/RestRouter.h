#pragma once

#include "net/RestEndpoint.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace http {
class Server;
struct Request;
struct Response;
} // namespace http

namespace net {
class RateLimiter;
} // namespace net

struct ServerSession;

/// Routes HTTP requests to registered RestEndpoint handlers.
/// Handles JSON parsing, bearer-token auth, error formatting, and
/// thread-safe dispatch (http runs handlers on its own thread pool).
class RestRouter {
  public:
    /// Given a bearer token, return the owning session or nullptr.
    using AuthFunc = std::function<ServerSession*(const std::string& token)>;

    void set_auth_func(AuthFunc func);
    void set_cors_origins(std::vector<std::string> origins);
    void set_rate_limiter(net::RateLimiter* limiter) {
        rate_limiter_ = limiter;
    }

    /// Takes ownership of an endpoint.
    void register_endpoint(std::unique_ptr<RestEndpoint> endpoint);

    /// Bind all registered endpoints to the http server.
    /// Call once after all endpoints have been registered.
    void bind(http::Server& server);

    /// Execute a callable under an exclusive dispatch lock.
    template <typename F>
    void with_lock(F&& func) {
        auto t0 = std::chrono::steady_clock::now();
        std::unique_lock lock(dispatch_mutex_);
        auto acquired = std::chrono::steady_clock::now();
        lock_stats.exclusive_acquisitions.fetch_add(1, std::memory_order_relaxed);
        lock_stats.exclusive_wait_ns.fetch_add(
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(acquired - t0).count()),
            std::memory_order_relaxed);
        func();
        lock_stats.exclusive_hold_ns.fetch_add(
            static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - acquired)
                    .count()),
            std::memory_order_relaxed);
    }

    /// Execute a callable under a shared (reader) dispatch lock.
    template <typename F>
    void with_shared_lock(F&& func) {
        auto t0 = std::chrono::steady_clock::now();
        std::shared_lock lock(dispatch_mutex_);
        auto acquired = std::chrono::steady_clock::now();
        lock_stats.shared_acquisitions.fetch_add(1, std::memory_order_relaxed);
        lock_stats.shared_wait_ns.fetch_add(
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(acquired - t0).count()),
            std::memory_order_relaxed);
        func();
    }

    /// Try to execute a callable under a shared lock.
    template <typename F>
    bool try_shared_lock(F&& func) {
        std::shared_lock lock(dispatch_mutex_, std::try_to_lock);
        if (!lock.owns_lock())
            return false;
        func();
        return true;
    }

    /// Dispatch lock contention stats (read by metrics thread).
    struct LockStats {
        std::atomic<uint64_t> exclusive_acquisitions{0};
        std::atomic<uint64_t> exclusive_wait_ns{0};
        std::atomic<uint64_t> exclusive_hold_ns{0};
        std::atomic<uint64_t> shared_acquisitions{0};
        std::atomic<uint64_t> shared_wait_ns{0};
    } lock_stats;

  private:
    void dispatch(RestEndpoint& endpoint, const http::Request& req, http::Response& res);
    void apply_cors_origin(const http::Request& req, http::Response& res) const;
    void apply_cors_for_endpoint(const RestEndpoint& endpoint, const http::Request& req, http::Response& res) const;
    bool cors_enabled() const;

    AuthFunc auth_func_;
    std::vector<std::string> cors_origins_;
    bool cors_wildcard_ = false;
    std::vector<std::unique_ptr<RestEndpoint>> endpoints_;
    std::shared_mutex dispatch_mutex_;
    net::RateLimiter* rate_limiter_ = nullptr;
};
