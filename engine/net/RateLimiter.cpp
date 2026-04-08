#include "net/RateLimiter.h"

#include "metrics/MetricsRegistry.h"

#include <algorithm>

static auto& ratelimit_rejected_ = metrics::MetricsRegistry::instance().counter("kagami_ratelimit_rejected_total",
                                                                                "Rate-limited rejections", {"action"});

namespace net {

void RateLimiter::configure(const std::string& action, RateLimitRule rule) {
    std::lock_guard lock(shards_mutex_);
    auto it = shards_.find(action);
    if (it == shards_.end()) {
        auto shard = std::make_unique<ActionShard>();
        shard->rule = rule;
        shards_.emplace(action, std::move(shard));
    }
    else {
        std::lock_guard shard_lock(it->second->mutex);
        it->second->rule = rule;
    }
}

bool RateLimiter::allow(const std::string& action, const std::string& key, double cost) {
    auto* shard = find_shard(action);
    if (!shard)
        return true; // Unconfigured action — allow by default.

    std::lock_guard lock(shard->mutex);
    auto now = std::chrono::steady_clock::now();
    auto [it, inserted] = shard->buckets.try_emplace(key);
    auto& bucket = it->second;

    if (inserted) {
        // New bucket — initialize from the action's rule.
        bucket.rate = shard->rule.rate;
        bucket.max_tokens = shard->rule.burst;
        bucket.tokens = shard->rule.burst;
        bucket.last_refill = now;
    }

    // Refill tokens based on elapsed time.
    double elapsed = std::chrono::duration<double>(now - bucket.last_refill).count();
    bucket.tokens = std::min(bucket.max_tokens, bucket.tokens + elapsed * bucket.rate);
    bucket.last_refill = now;

    if (bucket.tokens >= cost) {
        bucket.tokens -= cost;
        return true;
    }

    ratelimit_rejected_.labels({action}).inc();
    return false;
}

size_t RateLimiter::sweep(std::chrono::steady_clock::duration max_idle) {
    std::lock_guard lock(shards_mutex_);
    auto now = std::chrono::steady_clock::now();
    size_t evicted = 0;

    for (auto& [_, shard] : shards_) {
        std::lock_guard shard_lock(shard->mutex);
        for (auto it = shard->buckets.begin(); it != shard->buckets.end();) {
            if (now - it->second.last_refill > max_idle) {
                it = shard->buckets.erase(it);
                ++evicted;
            }
            else {
                ++it;
            }
        }
    }
    return evicted;
}

size_t RateLimiter::action_count() const {
    std::lock_guard lock(shards_mutex_);
    return shards_.size();
}

size_t RateLimiter::bucket_count() const {
    std::lock_guard lock(shards_mutex_);
    size_t count = 0;
    for (auto& [_, shard] : shards_) {
        std::lock_guard shard_lock(shard->mutex);
        count += shard->buckets.size();
    }
    return count;
}

RateLimiter::ActionShard* RateLimiter::get_or_create_shard(const std::string& action) {
    std::lock_guard lock(shards_mutex_);
    auto [it, inserted] = shards_.try_emplace(action);
    if (inserted)
        it->second = std::make_unique<ActionShard>();
    return it->second.get();
}

RateLimiter::ActionShard* RateLimiter::find_shard(const std::string& action) const {
    std::lock_guard lock(shards_mutex_);
    auto it = shards_.find(action);
    return it != shards_.end() ? it->second.get() : nullptr;
}

} // namespace net
