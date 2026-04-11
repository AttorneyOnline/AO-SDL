#include "moderation/RemoteDedupCache.h"

#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace {

using moderation::RemoteClassifierResult;
using moderation::RemoteDedupCache;

// A test clock lets us drive TTL expiry without sleeping. Mirrors
// the pattern in test_ModerationHeat.cpp.
int64_t g_dedup_clock_ms = 1'000'000'000'000LL;
int64_t dedup_fake_clock() {
    return g_dedup_clock_ms;
}

RemoteClassifierResult make_result(double hate) {
    RemoteClassifierResult r;
    r.ok = true;
    r.http_status = 200;
    r.duration_ms = 42;
    r.scores.hate = hate;
    return r;
}

class RemoteDedupCacheTest : public ::testing::Test {
  protected:
    void SetUp() override {
        g_dedup_clock_ms = 1'000'000'000'000LL;
        cache_.set_clock_for_tests(dedup_fake_clock);
        cache_.configure(/*max_entries=*/4, /*ttl_seconds=*/60);
    }
    void advance_seconds(double s) {
        g_dedup_clock_ms += static_cast<int64_t>(s * 1000.0);
    }
    RemoteDedupCache cache_;
};

} // namespace

TEST_F(RemoteDedupCacheTest, EmptyCacheMisses) {
    EXPECT_FALSE(cache_.get("nope").has_value());
    EXPECT_EQ(cache_.size(), 0u);
}

TEST_F(RemoteDedupCacheTest, StoreAndRetrieveRoundTrip) {
    cache_.put("k1", make_result(0.5));
    auto hit = cache_.get("k1");
    ASSERT_TRUE(hit.has_value());
    EXPECT_TRUE(hit->ok);
    EXPECT_DOUBLE_EQ(hit->scores.hate, 0.5);
}

TEST_F(RemoteDedupCacheTest, ExpiredEntryIsEvicted) {
    cache_.put("k1", make_result(0.5));
    advance_seconds(61.0); // past the 60s TTL
    EXPECT_FALSE(cache_.get("k1").has_value());
    // After the stale get(), the entry should have been pruned in place.
    EXPECT_EQ(cache_.size(), 0u);
}

TEST_F(RemoteDedupCacheTest, FreshEntryIsNotEvicted) {
    cache_.put("k1", make_result(0.5));
    advance_seconds(59.0); // just under TTL
    auto hit = cache_.get("k1");
    ASSERT_TRUE(hit.has_value());
    EXPECT_DOUBLE_EQ(hit->scores.hate, 0.5);
}

TEST_F(RemoteDedupCacheTest, EvictsOldestWhenFull) {
    // max_entries=4 from SetUp. Fill to capacity then overflow.
    cache_.put("k1", make_result(0.1));
    cache_.put("k2", make_result(0.2));
    cache_.put("k3", make_result(0.3));
    cache_.put("k4", make_result(0.4));
    EXPECT_EQ(cache_.size(), 4u);
    cache_.put("k5", make_result(0.5));
    // FIFO: k1 (oldest) evicted, k2-k5 remain.
    EXPECT_EQ(cache_.size(), 4u);
    EXPECT_FALSE(cache_.get("k1").has_value());
    EXPECT_TRUE(cache_.get("k2").has_value());
    EXPECT_TRUE(cache_.get("k5").has_value());
}

TEST_F(RemoteDedupCacheTest, PutOfExistingKeyRefreshes) {
    cache_.put("k1", make_result(0.1));
    advance_seconds(30.0);
    cache_.put("k1", make_result(0.9)); // refresh with new value
    advance_seconds(45.0); // original would have expired at t=60
    auto hit = cache_.get("k1");
    ASSERT_TRUE(hit.has_value());
    // Second put refreshed the timestamp so we're at dt=45s from it,
    // within the 60s TTL window. Value is the refreshed one.
    EXPECT_DOUBLE_EQ(hit->scores.hate, 0.9);
}

TEST_F(RemoteDedupCacheTest, ClearEmptiesEverything) {
    cache_.put("k1", make_result(0.1));
    cache_.put("k2", make_result(0.2));
    cache_.clear();
    EXPECT_EQ(cache_.size(), 0u);
    EXPECT_FALSE(cache_.get("k1").has_value());
    EXPECT_FALSE(cache_.get("k2").has_value());
}

TEST_F(RemoteDedupCacheTest, NormalizeLowercasesAndTrims) {
    EXPECT_EQ(RemoteDedupCache::normalize("Hello World"), "hello world");
    EXPECT_EQ(RemoteDedupCache::normalize("  HELLO  "), "hello");
    EXPECT_EQ(RemoteDedupCache::normalize("hello\t\t\tworld"), "hello world");
    EXPECT_EQ(RemoteDedupCache::normalize("hello\nworld"), "hello world");
}

TEST_F(RemoteDedupCacheTest, NormalizePreservesInternalPunctuation) {
    // Unlike SlurFilter, the dedup cache's normalization should NOT
    // drop punctuation — "hello, world" and "hello world" are
    // semantically distinct and should not share a cache entry.
    EXPECT_NE(RemoteDedupCache::normalize("hello, world"), RemoteDedupCache::normalize("hello world"));
    EXPECT_EQ(RemoteDedupCache::normalize("hello, world"), "hello, world");
}

TEST_F(RemoteDedupCacheTest, NormalizePassesThroughNonAscii) {
    // Non-ASCII bytes are preserved as-is (we don't do Unicode
    // case mapping). Two messages differing only in Unicode case
    // will miss cache, which is a conservative miss rather than a
    // false hit.
    const std::string s = "héllo";
    EXPECT_EQ(RemoteDedupCache::normalize(s), s);
}

TEST_F(RemoteDedupCacheTest, ComputeKeyIsStable) {
    auto k1 = RemoteDedupCache::compute_key("Hello World");
    auto k2 = RemoteDedupCache::compute_key("hello world");
    auto k3 = RemoteDedupCache::compute_key("  HELLO   WORLD  ");
    EXPECT_EQ(k1, k2);
    EXPECT_EQ(k1, k3);
    // Sanity: different content, different key.
    EXPECT_NE(k1, RemoteDedupCache::compute_key("hello there"));
    // sha256 hex is 64 chars.
    EXPECT_EQ(k1.size(), 64u);
}

TEST_F(RemoteDedupCacheTest, ThreadSafetyStress) {
    // Spawn N threads doing randomized get/put. Correctness here is
    // "no crash, no TSAN race, no segfault". Not a hit-rate test.
    constexpr int kThreads = 8;
    constexpr int kOpsPerThread = 500;
    std::atomic<int> hits{0};
    std::atomic<int> misses{0};
    cache_.configure(/*max_entries=*/32, /*ttl_seconds=*/60);

    auto worker = [&](int tid) {
        for (int i = 0; i < kOpsPerThread; ++i) {
            std::string k = "k" + std::to_string((tid * 31 + i) % 50);
            if (i % 3 == 0) {
                cache_.put(k, make_result(0.1 * tid));
            }
            else {
                if (cache_.get(k))
                    ++hits;
                else
                    ++misses;
            }
        }
    };
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t)
        threads.emplace_back(worker, t);
    for (auto& t : threads)
        t.join();
    // Sanity checks: got SOME hits, didn't blow up.
    EXPECT_GT(hits.load() + misses.load(), 0);
    EXPECT_LE(cache_.size(), 32u);
}
