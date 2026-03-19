#include "event/EventChannel.h"
#include "event/UIEvent.h"

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
