#include "event/BackgroundEvent.h"
#include "event/ChatEvent.h"
#include "event/EventManager.h"
#include "event/UIEvent.h"

#include <gtest/gtest.h>
#include <thread>
#include <vector>

// Helper: drain all pending events from a channel so singleton state
// doesn't leak between tests.
template <typename T>
void drain(EventChannel<T>& ch) {
    while (ch.get_event()) {
    }
}

// ---------------------------------------------------------------------------
// Singleton identity
// ---------------------------------------------------------------------------

TEST(EventManager, InstanceReturnsSameObject) {
    EventManager& a = EventManager::instance();
    EventManager& b = EventManager::instance();
    EXPECT_EQ(&a, &b);
}

// ---------------------------------------------------------------------------
// get_channel — same type returns same reference
// ---------------------------------------------------------------------------

TEST(EventManager, GetChannelReturnsSameReferenceForSameType) {
    auto& ch1 = EventManager::instance().get_channel<UIEvent>();
    auto& ch2 = EventManager::instance().get_channel<UIEvent>();
    EXPECT_EQ(&ch1, &ch2);

    drain(ch1);
}

// ---------------------------------------------------------------------------
// get_channel — different types return different channels
// ---------------------------------------------------------------------------

TEST(EventManager, GetChannelReturnsDifferentChannelsForDifferentTypes) {
    auto& ui_ch = EventManager::instance().get_channel<UIEvent>();
    auto& chat_ch = EventManager::instance().get_channel<ChatEvent>();
    auto& bg_ch = EventManager::instance().get_channel<BackgroundEvent>();

    // The addresses must all differ (they are distinct objects).
    EXPECT_NE(static_cast<void*>(&ui_ch), static_cast<void*>(&chat_ch));
    EXPECT_NE(static_cast<void*>(&chat_ch), static_cast<void*>(&bg_ch));
    EXPECT_NE(static_cast<void*>(&ui_ch), static_cast<void*>(&bg_ch));

    drain(ui_ch);
    drain(chat_ch);
    drain(bg_ch);
}

// ---------------------------------------------------------------------------
// Publishing to one channel doesn't affect another
// ---------------------------------------------------------------------------

TEST(EventManager, PublishingToOneChannelDoesNotAffectAnother) {
    auto& chat_ch = EventManager::instance().get_channel<ChatEvent>();
    auto& bg_ch = EventManager::instance().get_channel<BackgroundEvent>();

    // Drain any leftovers from prior tests.
    drain(chat_ch);
    drain(bg_ch);

    chat_ch.publish(ChatEvent("Alice", "Hello", false));

    EXPECT_TRUE(chat_ch.has_events());
    EXPECT_FALSE(bg_ch.has_events());

    drain(chat_ch);
}

// ---------------------------------------------------------------------------
// snapshot_channel_stats includes used channels
// ---------------------------------------------------------------------------

TEST(EventManager, SnapshotIncludesUsedChannels) {
    // Touch the BackgroundEvent channel so it exists in the registry.
    auto& bg_ch = EventManager::instance().get_channel<BackgroundEvent>();
    drain(bg_ch);

    auto stats = EventManager::instance().snapshot_channel_stats();

    // We should see at least BackgroundEvent (possibly more from other tests).
    EXPECT_FALSE(stats.empty());

    // Verify that a BackgroundEvent entry is present by looking for its
    // mangled type name.
    const char* expected_name = typeid(BackgroundEvent).name();
    bool found = false;
    for (const auto& s : stats) {
        if (std::string(s.raw_name) == std::string(expected_name)) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "BackgroundEvent channel not found in stats snapshot";
}

// ---------------------------------------------------------------------------
// Stats count reflects number of publishes
// ---------------------------------------------------------------------------

TEST(EventManager, StatsCountReflectsPublishCount) {
    auto& bg_ch = EventManager::instance().get_channel<BackgroundEvent>();

    // Record the count before our publishes.
    uint64_t before = bg_ch.publish_count();

    constexpr int N = 7;
    for (int i = 0; i < N; ++i) {
        bg_ch.publish(BackgroundEvent("bg_" + std::to_string(i), "pos"));
    }

    // Channel's own counter should have increased by N.
    EXPECT_EQ(bg_ch.publish_count(), before + N);

    // The snapshot should agree.
    const char* expected_name = typeid(BackgroundEvent).name();
    auto stats = EventManager::instance().snapshot_channel_stats();
    for (const auto& s : stats) {
        if (std::string(s.raw_name) == std::string(expected_name)) {
            EXPECT_EQ(s.count, before + N);
            break;
        }
    }

    drain(bg_ch);
}

// ---------------------------------------------------------------------------
// Stats are sorted by count descending
// ---------------------------------------------------------------------------

TEST(EventManager, StatsAreSortedDescending) {
    auto stats = EventManager::instance().snapshot_channel_stats();

    for (size_t i = 1; i < stats.size(); ++i) {
        EXPECT_GE(stats[i - 1].count, stats[i].count) << "Stats not sorted descending at index " << i;
    }
}

// ---------------------------------------------------------------------------
// Thread safety — concurrent get_channel calls don't crash
// ---------------------------------------------------------------------------

TEST(EventManager, ConcurrentGetChannelDoesNotCrash) {
    constexpr int NUM_THREADS = 8;
    constexpr int ITERATIONS = 500;

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < ITERATIONS; ++i) {
                // Each iteration touches all three channel types.
                auto& ui_ch = EventManager::instance().get_channel<UIEvent>();
                auto& chat_ch = EventManager::instance().get_channel<ChatEvent>();
                auto& bg_ch = EventManager::instance().get_channel<BackgroundEvent>();
                (void)ui_ch;
                (void)chat_ch;
                (void)bg_ch;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // If we get here without crashing or deadlocking, the test passes.
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Thread safety — concurrent publish and consume through the manager
// ---------------------------------------------------------------------------

TEST(EventManager, ConcurrentPublishAndConsumeViaManager) {
    auto& ch = EventManager::instance().get_channel<ChatEvent>();
    drain(ch);

    constexpr int N = 1000;

    std::thread producer([&] {
        for (int i = 0; i < N; ++i) {
            ch.publish(ChatEvent("sender", "msg_" + std::to_string(i), false));
        }
    });

    int consumed = 0;
    while (consumed < N) {
        if (ch.get_event()) {
            ++consumed;
        }
    }

    producer.join();
    EXPECT_EQ(consumed, N);

    drain(ch);
}
