/**
 * @file RateLimiter.h
 * @brief Token bucket rate limiter with per-action sharding.
 *
 * Provides rate limiting across multiple named actions (e.g. "session_create",
 * "ws_frame", "ao:MS"). Each action has its own mutex — checking one action
 * never contends with checking another.
 *
 * Thread-safe: all methods may be called from any thread.
 */
#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace net {

/// Configuration for a single rate-limited action.
struct RateLimitRule {
    double rate = 10.0;  ///< Tokens per second (sustained throughput).
    double burst = 20.0; ///< Maximum tokens (bucket capacity / peak tolerance).
};

/// Token bucket rate limiter with per-action sharded locks.
///
/// Usage:
///   RateLimiter rl;
///   rl.configure("session_create", {2.0, 5.0});
///   if (!rl.allow("session_create", client_ip))
///       return error_429();
class RateLimiter {
  public:
    RateLimiter() = default;

    /// Configure a named action. Creates the shard if it doesn't exist.
    /// Safe to call at any time (hot-reload).
    void configure(const std::string& action, RateLimitRule rule);

    /// Check if an action is allowed for the given key.
    /// Returns true and consumes `cost` tokens if allowed.
    /// Returns false if the bucket is empty (rate limited).
    /// Unconfigured actions are always allowed.
    bool allow(const std::string& action, const std::string& key, double cost = 1.0);

    /// Remove buckets that have been idle for longer than `max_idle`.
    /// Returns the number of buckets evicted. Call periodically (e.g. every 30s).
    size_t sweep(std::chrono::steady_clock::duration max_idle);

    /// Return the number of configured actions.
    size_t action_count() const;

    /// Return the total number of active buckets across all actions.
    size_t bucket_count() const;

  private:
    struct Bucket {
        double tokens;
        double max_tokens;
        double rate;
        std::chrono::steady_clock::time_point last_refill;
    };

    struct ActionShard {
        mutable std::mutex mutex;
        RateLimitRule rule;
        std::unordered_map<std::string, Bucket> buckets;
    };

    mutable std::mutex shards_mutex_; ///< Protects the shards_ map structure.
    std::unordered_map<std::string, std::unique_ptr<ActionShard>> shards_;

    ActionShard* get_or_create_shard(const std::string& action);
    ActionShard* find_shard(const std::string& action) const;
};

} // namespace net
