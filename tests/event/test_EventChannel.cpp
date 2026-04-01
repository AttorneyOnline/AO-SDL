#include "event/EventChannel.h"
#include "event/UIEvent.h"

#include <atomic>
#include <gtest/gtest.h>
#include <optional>
#include <thread>
#include <vector>

// EventChannel<T> is a header-only template; we test it via UIEvent.

TEST(EventChannel, EmptyChannelReturnsNullopt) {
    EventChannel<UIEvent> ch;
    EXPECT_EQ(ch.get_event(), std::nullopt);
}

TEST(EventChannel, HasEventsReturnsFalseWhenEmpty) {
    EventChannel<UIEvent> ch;
    EXPECT_FALSE(ch.has_events());
}

TEST(EventChannel, PublishedEventIsRetrievable) {
    EventChannel<UIEvent> ch;
    ch.publish(UIEvent(CHAR_LOADING_DONE));
    ASSERT_TRUE(ch.has_events());
    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_type(), CHAR_LOADING_DONE);
}

TEST(EventChannel, FifoOrdering) {
    EventChannel<UIEvent> ch;
    ch.publish(UIEvent(CHAR_LOADING_DONE));
    ch.publish(UIEvent(ENTERED_COURTROOM));

    auto first = ch.get_event();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->get_type(), CHAR_LOADING_DONE);

    auto second = ch.get_event();
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->get_type(), ENTERED_COURTROOM);
}

TEST(EventChannel, EmptyAfterDraining) {
    EventChannel<UIEvent> ch;
    ch.publish(UIEvent(CHAR_LOADING_DONE));
    ch.get_event();
    EXPECT_FALSE(ch.has_events());
    EXPECT_EQ(ch.get_event(), std::nullopt);
}

TEST(EventChannel, MultipleEventsCanBePublished) {
    EventChannel<UIEvent> ch;
    for (int i = 0; i < 5; ++i) {
        ch.publish(UIEvent(CHAR_LOADING_DONE));
    }
    int count = 0;
    while (ch.get_event()) {
        ++count;
    }
    EXPECT_EQ(count, 5);
}

TEST(EventChannel, ThreadSafePublishAndConsume) {
    EventChannel<UIEvent> ch;
    constexpr int N = 1000;

    std::thread producer([&] {
        for (int i = 0; i < N; ++i) {
            ch.publish(UIEvent(CHAR_LOADING_DONE));
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
}

// ===========================================================================
// on_publish callback
// ===========================================================================

TEST(EventChannel, OnPublishCalledOnEveryPublish) {
    EventChannel<UIEvent> ch;
    int call_count = 0;
    ch.set_on_publish([&call_count] { ++call_count; });

    ch.publish(UIEvent(CHAR_LOADING_DONE));
    EXPECT_EQ(call_count, 1);

    ch.publish(UIEvent(ENTERED_COURTROOM));
    EXPECT_EQ(call_count, 2);

    ch.publish(UIEvent(CHAR_LOADING_DONE));
    EXPECT_EQ(call_count, 3);
}

TEST(EventChannel, OnPublishNotCalledWithoutCallback) {
    EventChannel<UIEvent> ch;
    // No set_on_publish — should not crash
    ch.publish(UIEvent(CHAR_LOADING_DONE));
    ch.publish(UIEvent(ENTERED_COURTROOM));
}

TEST(EventChannel, OnPublishCanBeCleared) {
    EventChannel<UIEvent> ch;
    int call_count = 0;
    ch.set_on_publish([&call_count] { ++call_count; });

    ch.publish(UIEvent(CHAR_LOADING_DONE));
    EXPECT_EQ(call_count, 1);

    ch.set_on_publish(nullptr);
    ch.publish(UIEvent(ENTERED_COURTROOM));
    EXPECT_EQ(call_count, 1); // unchanged
}

TEST(EventChannel, OnPublishCalledOutsideLock) {
    // Verify the callback can safely call has_events() / get_event()
    // without deadlocking. This works because on_publish_ is invoked
    // outside the channel's mutex.
    EventChannel<UIEvent> ch;
    bool has_events_in_cb = false;
    ch.set_on_publish([&ch, &has_events_in_cb] { has_events_in_cb = ch.has_events(); });

    ch.publish(UIEvent(CHAR_LOADING_DONE));
    // The callback runs after the event is enqueued and the lock is released,
    // so has_events() should return true.
    EXPECT_TRUE(has_events_in_cb);
}

TEST(EventChannel, OnPublishReplacesCallback) {
    EventChannel<UIEvent> ch;
    int first_count = 0;
    int second_count = 0;

    ch.set_on_publish([&first_count] { ++first_count; });
    ch.publish(UIEvent(CHAR_LOADING_DONE));
    EXPECT_EQ(first_count, 1);
    EXPECT_EQ(second_count, 0);

    ch.set_on_publish([&second_count] { ++second_count; });
    ch.publish(UIEvent(ENTERED_COURTROOM));
    EXPECT_EQ(first_count, 1); // unchanged
    EXPECT_EQ(second_count, 1);
}

TEST(EventChannel, OnPublishThreadSafe) {
    EventChannel<UIEvent> ch;
    std::atomic<int> call_count{0};
    ch.set_on_publish([&call_count] { call_count.fetch_add(1); });

    constexpr int N = 500;
    std::thread t1([&] {
        for (int i = 0; i < N; ++i)
            ch.publish(UIEvent(CHAR_LOADING_DONE));
    });
    std::thread t2([&] {
        for (int i = 0; i < N; ++i)
            ch.publish(UIEvent(ENTERED_COURTROOM));
    });

    t1.join();
    t2.join();
    EXPECT_EQ(call_count.load(), N * 2);
}
