/**
 * @file RemoteDedupCache.h
 * @brief Short-lived cache for RemoteClassifier verdicts.
 *
 * Purpose: eliminate duplicate OpenAI moderation API calls for repeat
 * messages. In practice a chat server sees surprising amounts of
 * repeat traffic — spam waves, template responses, echo-chamber
 * chatter, copy-pasted meme lines — and sending each one through
 * omni-moderation burns API budget for no new information.
 *
 * Design choices:
 *
 * 1. Keyed by `sha256(normalize(message))`. The normalization is
 *    intentionally light: ASCII-lowercase + whitespace trim + internal
 *    whitespace collapse. This catches "HELLO", "hello ", "  hello  "
 *    as equal but does NOT aggressively fold letters like the slur
 *    filter does — the slur-folding normalization collapses too many
 *    semantically distinct messages into the same key, which would
 *    hand out stale verdicts. Light normalization keeps the false-
 *    hit rate near zero while still catching the common dedup targets.
 *
 * 2. FIFO eviction, not LRU. For cache patterns dominated by short-
 *    bursty repeats (spam waves) and long-tail distinct messages
 *    (normal chat), FIFO and LRU have indistinguishable hit rates.
 *    FIFO is simpler and lock-friendlier: a single deque of insertion
 *    order plus a hash map, no per-lookup reordering.
 *
 * 3. Explicit TTL per entry. Set once at insert, checked on every
 *    lookup. Entries past TTL are treated as cache misses (and the
 *    stale entry is pruned in place). This caps how long a cached
 *    verdict can live regardless of cache pressure, which matters
 *    because the remote classifier can change its verdict for the
 *    same text over time (policy updates, model retraining).
 *
 * 4. Thread-safe via a single std::mutex. Volume is low (one op
 *    per moderation check) and critical sections are short (map
 *    lookup + maybe one eviction), so contention is negligible
 *    under realistic load. If this ever becomes a hot path, a
 *    concurrent_unordered_map would be the next step.
 *
 * Not a general-purpose cache. Intentionally single-purpose: its
 * only consumer is RemoteClassifier::classify(), and its type
 * (RemoteClassifierResult) is tailored to that one callsite.
 */
#pragma once

#include "moderation/RemoteClassifier.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace moderation {

class RemoteDedupCache {
  public:
    /// Clock type for tests to inject a fake monotonic source.
    using Clock = int64_t (*)();

    RemoteDedupCache() = default;

    /// Light normalization: ASCII-lowercase, trim, collapse internal
    /// whitespace. Exposed as a static free function so tests can
    /// round-trip it without a cache instance.
    static std::string normalize(std::string_view input);

    /// Compute the cache key for @p text: sha256 over normalize(text),
    /// hex-encoded.
    static std::string compute_key(std::string_view text);

    /// Configure cache capacity and TTL. Calling this on a populated
    /// cache retains existing entries; they expire on their own TTL.
    void configure(size_t max_entries, int ttl_seconds);

    /// Look up a cached result. Returns nullopt on miss or stale
    /// entry. A stale entry is pruned in place before returning.
    std::optional<RemoteClassifierResult> get(const std::string& key);

    /// Insert a result under the given key. Evicts the oldest entry
    /// if the cache is full.
    void put(const std::string& key, const RemoteClassifierResult& result);

    /// Clear all cached entries. Retained for tests and /admin/reload
    /// paths. Not called on a regular schedule.
    void clear();

    /// Current number of entries (post any expiry pruning from the
    /// last get() call). Intended for metrics / introspection.
    size_t size() const;

    /// For tests: override the clock source. Function returns
    /// monotonic milliseconds. Absolute value is not meaningful —
    /// only deltas matter for TTL checks.
    void set_clock_for_tests(Clock clock) {
        clock_ = clock;
    }

  private:
    struct Entry {
        RemoteClassifierResult result;
        int64_t inserted_ms = 0;
    };

    int64_t now_ms() const;

    mutable std::mutex mu_;
    std::unordered_map<std::string, Entry> map_;
    std::deque<std::string> insertion_order_;
    size_t max_entries_ = 1000;
    int64_t ttl_ms_ = 300000; // 5 minutes default
    Clock clock_ = nullptr;
};

} // namespace moderation
