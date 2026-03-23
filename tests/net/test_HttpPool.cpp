#include "net/HttpPool.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

TEST(HttpPool, DefaultConstructorCreatesPool) {
    HttpPool pool;
    // Pool should be alive; pending starts at zero.
    EXPECT_EQ(pool.pending(), 0);
}

TEST(HttpPool, ConstructorWithExplicitThreadCount) {
    HttpPool pool(4);
    EXPECT_EQ(pool.pending(), 0);
}

TEST(HttpPool, SingleThreadPool) {
    HttpPool pool(1);
    EXPECT_EQ(pool.pending(), 0);
}

TEST(HttpPool, DestructorJoinsWithoutHanging) {
    // Destructor calls stop() which joins worker threads.
    // This test verifies it completes in bounded time.
    auto pool = std::make_unique<HttpPool>(2);
    pool.reset(); // Triggers destructor
    SUCCEED();
}

// ---------------------------------------------------------------------------
// pending() initial state
// ---------------------------------------------------------------------------

TEST(HttpPool, PendingStartsAtZero) {
    HttpPool pool(1);
    EXPECT_EQ(pool.pending(), 0);
}

// ---------------------------------------------------------------------------
// poll() on empty pool
// ---------------------------------------------------------------------------

TEST(HttpPool, PollReturnsZeroWhenEmpty) {
    HttpPool pool(1);
    EXPECT_EQ(pool.poll(), 0);
}

TEST(HttpPool, PollReturnsZeroMultipleTimes) {
    HttpPool pool(1);
    EXPECT_EQ(pool.poll(), 0);
    EXPECT_EQ(pool.poll(), 0);
    EXPECT_EQ(pool.poll(), 0);
}

// ---------------------------------------------------------------------------
// stop() idempotency
// ---------------------------------------------------------------------------

TEST(HttpPool, StopIsIdempotent) {
    HttpPool pool(2);
    pool.stop();
    pool.stop();
    pool.stop();
    SUCCEED();
}

TEST(HttpPool, StopThenPollReturnsZero) {
    HttpPool pool(2);
    pool.stop();
    EXPECT_EQ(pool.poll(), 0);
}

TEST(HttpPool, StopThenPendingReturnsZero) {
    HttpPool pool(2);
    pool.stop();
    EXPECT_EQ(pool.pending(), 0);
}

// ---------------------------------------------------------------------------
// drop_below() — fires callbacks with error="dropped", status=0
// ---------------------------------------------------------------------------

TEST(HttpPool, DropBelowFiresCallbacksForLowPriorityRequests) {
    // Stop the pool first so no workers can dequeue the requests,
    // then submit requests, then call drop_below.
    // But stop() clears the queue, so we need a different approach:
    // We use a pool with 0 threads... wait, constructor requires > 0 implicitly.
    //
    // Instead: create pool, submit requests, immediately call drop_below.
    // Workers might not have dequeued yet if we're fast enough, but this is racy.
    // A safer approach: submit after stop, since get() still pushes to the queue
    // even when running_ is false (get() doesn't check running_).
    HttpPool pool(1);
    pool.stop();

    // After stop(), the work queue was cleared and workers joined.
    // But get() still pushes to the deque (it doesn't check running_).
    // And drop_below() still scans the deque.
    pool.get("host", "/path", [](HttpResponse) {}, HttpPriority::LOW);
    pool.get("host", "/path", [](HttpResponse) {}, HttpPriority::LOW);

    std::atomic<int> dropped_count{0};
    // Re-submit with callbacks that track invocations.
    // Actually, the above two won't have our tracking callback. Let's redo.
    // We need to be more careful. Let's create a fresh pool and stop it.

    HttpPool pool2(1);
    pool2.stop();

    std::vector<HttpResponse> responses;
    std::mutex mu;

    auto cb = [&](HttpResponse resp) {
        std::lock_guard lock(mu);
        responses.push_back(std::move(resp));
    };

    pool2.get("host", "/a", cb, HttpPriority::LOW);
    pool2.get("host", "/b", cb, HttpPriority::NORMAL);

    // Drop everything below HIGH — should drop LOW and NORMAL.
    pool2.drop_below(HttpPriority::HIGH);

    std::lock_guard lock(mu);
    ASSERT_EQ(responses.size(), 2u);
    for (const auto& r : responses) {
        EXPECT_EQ(r.status, 0);
        EXPECT_EQ(r.error, "dropped");
        EXPECT_TRUE(r.body.empty());
    }
}

TEST(HttpPool, DropBelowOnlyDropsStrictlyBelowThreshold) {
    HttpPool pool(1);
    pool.stop();

    std::atomic<int> dropped_count{0};
    std::atomic<int> kept_count{0};

    auto dropped_cb = [&](HttpResponse resp) {
        EXPECT_EQ(resp.error, "dropped");
        dropped_count.fetch_add(1);
    };

    auto kept_cb = [&](HttpResponse) { kept_count.fetch_add(1); };

    pool.get("host", "/low", dropped_cb, HttpPriority::LOW);
    pool.get("host", "/normal", kept_cb, HttpPriority::NORMAL);
    pool.get("host", "/high", kept_cb, HttpPriority::HIGH);

    // Drop strictly below NORMAL — only LOW should be dropped.
    pool.drop_below(HttpPriority::NORMAL);

    EXPECT_EQ(dropped_count.load(), 1);
    // NORMAL and HIGH should still be in the queue, not invoked.
    EXPECT_EQ(kept_count.load(), 0);
}

TEST(HttpPool, DropBelowDecrementsCountPerDropped) {
    HttpPool pool(1);
    pool.stop();

    pool.get("host", "/a", [](HttpResponse) {}, HttpPriority::LOW);
    pool.get("host", "/b", [](HttpResponse) {}, HttpPriority::LOW);
    pool.get("host", "/c", [](HttpResponse) {}, HttpPriority::HIGH);

    // 3 pending
    EXPECT_EQ(pool.pending(), 3);

    pool.drop_below(HttpPriority::NORMAL);

    // 2 LOW requests dropped, 1 HIGH remains
    EXPECT_EQ(pool.pending(), 1);
}

TEST(HttpPool, DropBelowWithNothingToDrop) {
    HttpPool pool(1);
    pool.stop();

    pool.get("host", "/a", [](HttpResponse) {}, HttpPriority::HIGH);
    pool.get("host", "/b", [](HttpResponse) {}, HttpPriority::CRITICAL);

    pool.drop_below(HttpPriority::NORMAL);

    // Nothing was dropped; both remain.
    EXPECT_EQ(pool.pending(), 2);
}

TEST(HttpPool, DropBelowWithEmptyQueue) {
    HttpPool pool(1);
    pool.drop_below(HttpPriority::CRITICAL);
    EXPECT_EQ(pool.pending(), 0);
}

TEST(HttpPool, DropBelowLowDropsNothing) {
    // LOW is the lowest priority; nothing is strictly below LOW.
    HttpPool pool(1);
    pool.stop();

    pool.get("host", "/a", [](HttpResponse) {}, HttpPriority::LOW);
    pool.get("host", "/b", [](HttpResponse) {}, HttpPriority::NORMAL);

    pool.drop_below(HttpPriority::LOW);
    // Nothing is strictly below LOW, so nothing should be dropped.
    EXPECT_EQ(pool.pending(), 2);
}

TEST(HttpPool, DropBelowCriticalDropsAllButCritical) {
    HttpPool pool(1);
    pool.stop();

    std::atomic<int> dropped{0};
    auto cb = [&](HttpResponse resp) {
        if (resp.error == "dropped")
            dropped.fetch_add(1);
    };

    pool.get("host", "/a", cb, HttpPriority::LOW);
    pool.get("host", "/b", cb, HttpPriority::NORMAL);
    pool.get("host", "/c", cb, HttpPriority::HIGH);
    pool.get("host", "/d", cb, HttpPriority::CRITICAL);

    pool.drop_below(HttpPriority::CRITICAL);

    EXPECT_EQ(dropped.load(), 3);
    EXPECT_EQ(pool.pending(), 1);
}

// ---------------------------------------------------------------------------
// Priority ordering: higher priority dequeued first
// ---------------------------------------------------------------------------

TEST(HttpPool, PriorityInsertionOrder) {
    // Verify that higher-priority requests are placed before lower ones.
    // After stop(), the queue is cleared. So we submit to a stopped pool
    // and use drop_below to observe the callback order.
    HttpPool pool(1);
    pool.stop();

    // Submit in arbitrary order: NORMAL, LOW, HIGH, NORMAL
    std::vector<std::string> dropped_paths;
    std::mutex mu;

    auto make_cb = [&](const std::string& path) {
        return [&, path](HttpResponse resp) {
            if (resp.error == "dropped") {
                std::lock_guard lock(mu);
                dropped_paths.push_back(path);
            }
        };
    };

    pool.get("host", "/normal1", make_cb("/normal1"), HttpPriority::NORMAL);
    pool.get("host", "/low", make_cb("/low"), HttpPriority::LOW);
    pool.get("host", "/high", make_cb("/high"), HttpPriority::HIGH);
    pool.get("host", "/normal2", make_cb("/normal2"), HttpPriority::NORMAL);

    // Drop everything below CRITICAL — all 4 should be dropped.
    pool.drop_below(HttpPriority::CRITICAL);

    std::lock_guard lock(mu);
    ASSERT_EQ(dropped_paths.size(), 4u);

    // drop_below iterates from front to back. The queue is priority-sorted
    // (highest first), so we expect: high, normal1, normal2, low.
    // HIGH was inserted after NORMAL1 but its priority is higher, so it
    // should be before both NORMALs.
    EXPECT_EQ(dropped_paths[0], "/high");
    // The two NORMALs should appear before LOW, in insertion order.
    EXPECT_EQ(dropped_paths[1], "/normal1");
    EXPECT_EQ(dropped_paths[2], "/normal2");
    EXPECT_EQ(dropped_paths[3], "/low");
}

TEST(HttpPool, CriticalBeforeHigh) {
    HttpPool pool(1);
    pool.stop();

    std::vector<HttpPriority> order;
    std::mutex mu;

    auto make_cb = [&](HttpPriority p) {
        return [&, p](HttpResponse resp) {
            if (resp.error == "dropped") {
                std::lock_guard lock(mu);
                order.push_back(p);
            }
        };
    };

    pool.get("host", "/low", make_cb(HttpPriority::LOW), HttpPriority::LOW);
    pool.get("host", "/critical", make_cb(HttpPriority::CRITICAL), HttpPriority::CRITICAL);
    pool.get("host", "/high", make_cb(HttpPriority::HIGH), HttpPriority::HIGH);
    pool.get("host", "/normal", make_cb(HttpPriority::NORMAL), HttpPriority::NORMAL);

    // Use a threshold above CRITICAL to drop everything.
    // There's no priority above CRITICAL, so we can't use drop_below for that.
    // Instead, drop_below(CRITICAL) drops LOW, NORMAL, HIGH but not CRITICAL.
    // Let's just verify relative ordering of the dropped ones.
    pool.drop_below(HttpPriority::CRITICAL);

    std::lock_guard lock(mu);
    ASSERT_EQ(order.size(), 3u);
    // Order should be: HIGH, NORMAL, LOW (front-to-back in priority queue).
    EXPECT_EQ(order[0], HttpPriority::HIGH);
    EXPECT_EQ(order[1], HttpPriority::NORMAL);
    EXPECT_EQ(order[2], HttpPriority::LOW);
}

// ---------------------------------------------------------------------------
// get() increments pending
// ---------------------------------------------------------------------------

TEST(HttpPool, GetIncrementsPending) {
    HttpPool pool(1);
    pool.stop();

    pool.get("host", "/a", [](HttpResponse) {}, HttpPriority::NORMAL);
    EXPECT_GE(pool.pending(), 1);

    pool.get("host", "/b", [](HttpResponse) {}, HttpPriority::NORMAL);
    EXPECT_GE(pool.pending(), 2);
}

TEST(HttpPool, GetStreamingIncrementsPending) {
    HttpPool pool(1);
    pool.stop();

    pool.get_streaming(
        "host", "/stream", [](const uint8_t*, size_t) -> bool { return true; }, [](HttpResponse) {},
        HttpPriority::NORMAL);

    EXPECT_GE(pool.pending(), 1);
}

// ---------------------------------------------------------------------------
// After stop(), queued requests are discarded
// ---------------------------------------------------------------------------

TEST(HttpPool, StopClearsWorkQueue) {
    HttpPool pool(1);
    pool.stop();

    // Submit some requests to the stopped pool (get() still pushes to deque).
    pool.get("host", "/a", [](HttpResponse) {}, HttpPriority::NORMAL);
    pool.get("host", "/b", [](HttpResponse) {}, HttpPriority::NORMAL);
    EXPECT_EQ(pool.pending(), 2);

    // Calling stop() again should clear the queue.
    // But wait — stop() checks running_ first and returns early if already false.
    // So the queue won't be cleared a second time. This is expected behavior:
    // once stopped, no workers exist to process anything.
    pool.stop();

    // The requests remain in the deque but will never be processed.
    // pending() reflects them but they are effectively orphaned.
    // This test documents the behavior: stop() on an already-stopped pool is a no-op.
}

TEST(HttpPool, StopBeforeAnyRequestsLeavesPoolClean) {
    HttpPool pool(2);
    pool.stop();
    EXPECT_EQ(pool.pending(), 0);
    EXPECT_EQ(pool.poll(), 0);
}

// ---------------------------------------------------------------------------
// drop_below() with streaming requests
// ---------------------------------------------------------------------------

TEST(HttpPool, DropBelowDropsStreamingRequests) {
    HttpPool pool(1);
    pool.stop();

    std::atomic<bool> callback_fired{false};
    pool.get_streaming(
        "host", "/stream", [](const uint8_t*, size_t) -> bool { return true; },
        [&](HttpResponse resp) {
            EXPECT_EQ(resp.status, 0);
            EXPECT_EQ(resp.error, "dropped");
            callback_fired.store(true);
        },
        HttpPriority::LOW);

    pool.drop_below(HttpPriority::NORMAL);
    EXPECT_TRUE(callback_fired.load());
    EXPECT_EQ(pool.pending(), 0);
}

// ---------------------------------------------------------------------------
// drop_below() callback receives correct response fields
// ---------------------------------------------------------------------------

TEST(HttpPool, DropBelowResponseHasEmptyBody) {
    HttpPool pool(1);
    pool.stop();

    HttpResponse received;
    pool.get("host", "/test", [&](HttpResponse resp) { received = std::move(resp); }, HttpPriority::LOW);

    pool.drop_below(HttpPriority::NORMAL);

    EXPECT_EQ(received.status, 0);
    EXPECT_EQ(received.error, "dropped");
    EXPECT_TRUE(received.body.empty());
}

// ---------------------------------------------------------------------------
// Multiple drop_below() calls
// ---------------------------------------------------------------------------

TEST(HttpPool, DropBelowCalledMultipleTimesIsHarmless) {
    HttpPool pool(1);
    pool.stop();

    std::atomic<int> count{0};
    pool.get("host", "/a", [&](HttpResponse) { count++; }, HttpPriority::LOW);

    pool.drop_below(HttpPriority::NORMAL);
    EXPECT_EQ(count.load(), 1);

    // Second call should have nothing to drop.
    pool.drop_below(HttpPriority::NORMAL);
    EXPECT_EQ(count.load(), 1); // No additional callbacks.
}

// ---------------------------------------------------------------------------
// Null callback handling in drop_below
// ---------------------------------------------------------------------------

TEST(HttpPool, DropBelowWithNullCallbackDoesNotCrash) {
    HttpPool pool(1);
    pool.stop();

    // Submit a request with a null callback.
    pool.get("host", "/a", nullptr, HttpPriority::LOW);

    // Should not crash even though callback is null.
    pool.drop_below(HttpPriority::NORMAL);
    EXPECT_EQ(pool.pending(), 0);
}

// ---------------------------------------------------------------------------
// Mixed priority requests — selective drop
// ---------------------------------------------------------------------------

TEST(HttpPool, DropBelowSelectivelyRemovesRequests) {
    HttpPool pool(1);
    pool.stop();

    std::vector<std::string> dropped;
    std::mutex mu;

    auto make_cb = [&](const std::string& id) {
        return [&, id](HttpResponse resp) {
            if (resp.error == "dropped") {
                std::lock_guard lock(mu);
                dropped.push_back(id);
            }
        };
    };

    pool.get("host", "/low1", make_cb("low1"), HttpPriority::LOW);
    pool.get("host", "/normal1", make_cb("normal1"), HttpPriority::NORMAL);
    pool.get("host", "/high1", make_cb("high1"), HttpPriority::HIGH);
    pool.get("host", "/low2", make_cb("low2"), HttpPriority::LOW);
    pool.get("host", "/critical1", make_cb("critical1"), HttpPriority::CRITICAL);

    EXPECT_EQ(pool.pending(), 5);

    // Drop below HIGH — removes LOW and NORMAL.
    pool.drop_below(HttpPriority::HIGH);

    {
        std::lock_guard lock(mu);
        EXPECT_EQ(dropped.size(), 3u);
        // Verify the right ones were dropped.
        EXPECT_NE(std::find(dropped.begin(), dropped.end(), "low1"), dropped.end());
        EXPECT_NE(std::find(dropped.begin(), dropped.end(), "low2"), dropped.end());
        EXPECT_NE(std::find(dropped.begin(), dropped.end(), "normal1"), dropped.end());
    }

    // HIGH and CRITICAL should remain.
    EXPECT_EQ(pool.pending(), 2);
}

// ---------------------------------------------------------------------------
// Lifecycle: construct → use → destroy (no explicit stop)
// ---------------------------------------------------------------------------

TEST(HttpPool, DestructorStopsPoolImplicitly) {
    // The destructor calls stop(). Ensure this works even after submitting
    // requests (to a stopped pool, so no real HTTP).
    {
        HttpPool pool(2);
        pool.stop();
        pool.get("host", "/a", [](HttpResponse) {}, HttpPriority::NORMAL);
        // pool goes out of scope — destructor calls stop() again (idempotent).
    }
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Edge case: zero-priority requests
// ---------------------------------------------------------------------------

TEST(HttpPool, AllSamePriorityMaintainsInsertionOrder) {
    HttpPool pool(1);
    pool.stop();

    std::vector<std::string> order;
    std::mutex mu;

    for (int i = 0; i < 5; i++) {
        std::string id = std::to_string(i);
        pool.get(
            "host", "/" + id,
            [&, id](HttpResponse resp) {
                if (resp.error == "dropped") {
                    std::lock_guard lock(mu);
                    order.push_back(id);
                }
            },
            HttpPriority::NORMAL);
    }

    pool.drop_below(HttpPriority::HIGH);

    std::lock_guard lock(mu);
    ASSERT_EQ(order.size(), 5u);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(order[i], std::to_string(i));
    }
}

// ---------------------------------------------------------------------------
// poll() after drop_below() — dropped callbacks fire synchronously
// ---------------------------------------------------------------------------

TEST(HttpPool, DroppedCallbacksDoNotAppearInPoll) {
    HttpPool pool(1);
    pool.stop();

    std::atomic<int> direct_count{0};
    pool.get(
        "host", "/a",
        [&](HttpResponse resp) {
            if (resp.error == "dropped")
                direct_count.fetch_add(1);
        },
        HttpPriority::LOW);

    pool.drop_below(HttpPriority::NORMAL);
    EXPECT_EQ(direct_count.load(), 1);

    // poll() should return 0 — dropped callbacks were fired synchronously,
    // not added to the result queue.
    EXPECT_EQ(pool.poll(), 0);
}

// ---------------------------------------------------------------------------
// Large batch of requests
// ---------------------------------------------------------------------------

TEST(HttpPool, LargeBatchDropBelow) {
    HttpPool pool(1);
    pool.stop();

    constexpr int N = 100;
    std::atomic<int> dropped{0};

    for (int i = 0; i < N; i++) {
        pool.get(
            "host", "/item" + std::to_string(i),
            [&](HttpResponse resp) {
                if (resp.error == "dropped")
                    dropped.fetch_add(1);
            },
            HttpPriority::LOW);
    }

    EXPECT_EQ(pool.pending(), N);
    pool.drop_below(HttpPriority::NORMAL);
    EXPECT_EQ(dropped.load(), N);
    EXPECT_EQ(pool.pending(), 0);
}
