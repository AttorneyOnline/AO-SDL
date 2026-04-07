#include <gtest/gtest.h>

#include "net/RateLimiter.h"

#include <thread>
#include <vector>

using namespace net;

TEST(RateLimiter, UnconfiguredActionAlwaysAllows) {
    RateLimiter rl;
    EXPECT_TRUE(rl.allow("unknown_action", "key1"));
    EXPECT_TRUE(rl.allow("unknown_action", "key1"));
    EXPECT_TRUE(rl.allow("unknown_action", "key1"));
}

TEST(RateLimiter, BurstThenReject) {
    RateLimiter rl;
    rl.configure("test", {10.0, 3.0}); // 10/sec, burst 3

    // Consume the burst
    EXPECT_TRUE(rl.allow("test", "key1"));
    EXPECT_TRUE(rl.allow("test", "key1"));
    EXPECT_TRUE(rl.allow("test", "key1"));

    // Bucket empty — reject
    EXPECT_FALSE(rl.allow("test", "key1"));
    EXPECT_FALSE(rl.allow("test", "key1"));
}

TEST(RateLimiter, DifferentKeysAreIndependent) {
    RateLimiter rl;
    rl.configure("test", {10.0, 2.0});

    EXPECT_TRUE(rl.allow("test", "alice"));
    EXPECT_TRUE(rl.allow("test", "alice"));
    EXPECT_FALSE(rl.allow("test", "alice")); // alice exhausted

    // bob is unaffected
    EXPECT_TRUE(rl.allow("test", "bob"));
    EXPECT_TRUE(rl.allow("test", "bob"));
    EXPECT_FALSE(rl.allow("test", "bob"));
}

TEST(RateLimiter, DifferentActionsAreIndependent) {
    RateLimiter rl;
    rl.configure("action_a", {10.0, 2.0});
    rl.configure("action_b", {10.0, 2.0});

    EXPECT_TRUE(rl.allow("action_a", "key"));
    EXPECT_TRUE(rl.allow("action_a", "key"));
    EXPECT_FALSE(rl.allow("action_a", "key")); // action_a exhausted

    // action_b is unaffected
    EXPECT_TRUE(rl.allow("action_b", "key"));
    EXPECT_TRUE(rl.allow("action_b", "key"));
    EXPECT_FALSE(rl.allow("action_b", "key"));
}

TEST(RateLimiter, RefillOverTime) {
    RateLimiter rl;
    rl.configure("test", {100.0, 2.0}); // 100/sec refill, burst 2

    EXPECT_TRUE(rl.allow("test", "key"));
    EXPECT_TRUE(rl.allow("test", "key"));
    EXPECT_FALSE(rl.allow("test", "key")); // empty

    // Wait 50ms — should refill ~5 tokens (100/sec * 0.05s), capped at burst=2
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_TRUE(rl.allow("test", "key"));
    EXPECT_TRUE(rl.allow("test", "key"));
    EXPECT_FALSE(rl.allow("test", "key")); // empty again
}

TEST(RateLimiter, CostParameter) {
    RateLimiter rl;
    rl.configure("bytes", {1000.0, 100.0}); // 1000 bytes/sec, burst 100

    // Use 60 of 100 tokens
    EXPECT_TRUE(rl.allow("bytes", "conn1", 60.0));
    // Use 30 more — 90 total, still under 100
    EXPECT_TRUE(rl.allow("bytes", "conn1", 30.0));
    // Try 20 more — would exceed 100, rejected
    EXPECT_FALSE(rl.allow("bytes", "conn1", 20.0));
    // But 10 fits exactly
    EXPECT_TRUE(rl.allow("bytes", "conn1", 10.0));
}

TEST(RateLimiter, SweepRemovesIdleBuckets) {
    RateLimiter rl;
    rl.configure("test", {10.0, 5.0});

    rl.allow("test", "active");
    rl.allow("test", "stale");

    EXPECT_EQ(rl.bucket_count(), 2u);

    // Sleep so both become "stale" from sweep's perspective with a short threshold
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Touch "active" to keep it alive
    rl.allow("test", "active");

    // Sweep with 30ms idle threshold — "stale" was last touched 50ms ago
    size_t evicted = rl.sweep(std::chrono::milliseconds(30));

    EXPECT_EQ(evicted, 1u);
    EXPECT_EQ(rl.bucket_count(), 1u);

    // "active" still works
    EXPECT_TRUE(rl.allow("test", "active"));
}

TEST(RateLimiter, ConfigureCounts) {
    RateLimiter rl;
    EXPECT_EQ(rl.action_count(), 0u);

    rl.configure("a", {1.0, 1.0});
    EXPECT_EQ(rl.action_count(), 1u);

    rl.configure("b", {1.0, 1.0});
    EXPECT_EQ(rl.action_count(), 2u);

    // Reconfigure existing action doesn't add a new one
    rl.configure("a", {2.0, 2.0});
    EXPECT_EQ(rl.action_count(), 2u);
}

TEST(RateLimiter, ReconfigureUpdatesRate) {
    RateLimiter rl;
    rl.configure("test", {10.0, 2.0});

    EXPECT_TRUE(rl.allow("test", "key"));
    EXPECT_TRUE(rl.allow("test", "key"));
    EXPECT_FALSE(rl.allow("test", "key")); // burst=2 exhausted

    // Reconfigure with higher burst
    rl.configure("test", {10.0, 5.0});

    // Existing bucket still has 0 tokens (not reset)
    EXPECT_FALSE(rl.allow("test", "key"));

    // But new keys get the new burst
    EXPECT_TRUE(rl.allow("test", "new_key"));
    EXPECT_TRUE(rl.allow("test", "new_key"));
    EXPECT_TRUE(rl.allow("test", "new_key"));
    EXPECT_TRUE(rl.allow("test", "new_key"));
    EXPECT_TRUE(rl.allow("test", "new_key"));
    EXPECT_FALSE(rl.allow("test", "new_key")); // burst=5 exhausted
}

TEST(RateLimiter, ThreadSafety) {
    RateLimiter rl;
    rl.configure("test", {1000.0, 100.0}); // 1000/sec, burst 100

    std::atomic<int> allowed{0};
    std::atomic<int> rejected{0};

    auto worker = [&]() {
        for (int i = 0; i < 200; ++i) {
            if (rl.allow("test", "shared_key"))
                allowed.fetch_add(1, std::memory_order_relaxed);
            else
                rejected.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i)
        threads.emplace_back(worker);
    for (auto& t : threads)
        t.join();

    // Total attempts: 4 * 200 = 800
    EXPECT_EQ(allowed.load() + rejected.load(), 800);
    // With burst=100, at least 100 should be allowed (the initial burst)
    EXPECT_GE(allowed.load(), 100);
    // But not all 800 (rate limited)
    EXPECT_LT(allowed.load(), 800);
}
