#include <gtest/gtest.h>

#include "platform/Poll.h"
#include "platform/Socket.h"

#include <atomic>
#include <chrono>
#include <set>
#include <thread>

using namespace platform;

// Helper: create a connected pair of sockets on loopback.
static std::pair<Socket, Socket> make_pair() {
    auto listener = tcp_listen("127.0.0.1", 0);
    uint16_t port = listener.local_port();

    auto client = tcp_connect("127.0.0.1", port);

    listener.set_non_blocking(false);
    std::string remote;
    uint16_t rp = 0;
    auto server = tcp_accept(listener, remote, rp);

    client.set_non_blocking(true);
    server.set_non_blocking(true);

    return {std::move(client), std::move(server)};
}

// -- Basic poll behavior ------------------------------------------------------

TEST(PlatformPoller, TimeoutWithNoEventsReturnsZero) {
    Poller poller;
    Poller::Event events[4];
    auto start = std::chrono::steady_clock::now();
    int n = poller.poll(events, 4, 50);
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_EQ(n, 0);
    // Should have waited at least ~40ms (some tolerance)
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 30);
}

TEST(PlatformPoller, NonBlockingPollReturnsImmediately) {
    Poller poller;
    Poller::Event events[4];
    auto start = std::chrono::steady_clock::now();
    int n = poller.poll(events, 4, 0);
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_EQ(n, 0);
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 10);
}

// -- Readable notification ----------------------------------------------------

TEST(PlatformPoller, DetectsReadableSocket) {
    auto [client, server] = make_pair();

    Poller poller;
    poller.add(server, Poller::Readable);

    // Nothing to read yet
    Poller::Event events[4];
    EXPECT_EQ(poller.poll(events, 4, 0), 0);

    // Write from client
    client.send("data", 4);

    // Now server should be readable
    int n = poller.poll(events, 4, 100);
    EXPECT_GE(n, 1);
    EXPECT_TRUE(events[0].flags & Poller::Readable);
}

TEST(PlatformPoller, MultipleSocketsReadable) {
    auto [c1, s1] = make_pair();
    auto [c2, s2] = make_pair();

    Poller poller;
    poller.add(s1, Poller::Readable, reinterpret_cast<void*>(1));
    poller.add(s2, Poller::Readable, reinterpret_cast<void*>(2));

    c1.send("a", 1);
    c2.send("b", 1);

    Poller::Event events[4];
    // May return 1 or 2 events depending on timing
    int total = 0;
    for (int attempt = 0; attempt < 5 && total < 2; ++attempt) {
        int n = poller.poll(events + total, 4 - total, 50);
        total += n;
    }
    EXPECT_EQ(total, 2);
}

// -- Remove -------------------------------------------------------------------

TEST(PlatformPoller, RemoveStopsNotifications) {
    auto [client, server] = make_pair();

    Poller poller;
    poller.add(server, Poller::Readable);
    poller.remove(server);

    client.send("data", 4);

    Poller::Event events[4];
    int n = poller.poll(events, 4, 50);
    EXPECT_EQ(n, 0);
}

// -- Modify -------------------------------------------------------------------

TEST(PlatformPoller, ModifyChangesInterest) {
    auto [client, server] = make_pair();

    Poller poller;
    poller.add(server, Poller::Writable); // initially interested in write only

    client.send("data", 4);

    Poller::Event events[4];
    // Should get writable but not readable
    int n = poller.poll(events, 4, 50);
    if (n > 0) {
        EXPECT_TRUE(events[0].flags & Poller::Writable);
    }

    // Now switch interest to readable only
    poller.modify(server, Poller::Readable);
    n = poller.poll(events, 4, 50);
    EXPECT_GE(n, 1);
    EXPECT_TRUE(events[0].flags & Poller::Readable);
}

// -- user_data passthrough ----------------------------------------------------

TEST(PlatformPoller, UserDataPassedThrough) {
    auto [client, server] = make_pair();

    int tag = 42;
    Poller poller;
    poller.add(server, Poller::Readable, &tag);

    client.send("x", 1);

    Poller::Event events[4];
    int n = poller.poll(events, 4, 100);
    ASSERT_GE(n, 1);
    EXPECT_EQ(events[0].user_data, &tag);
}

// -- fd-based overloads -------------------------------------------------------

TEST(PlatformPoller, FdOverloadWorks) {
    auto [client, server] = make_pair();

    Poller poller;
    poller.add(server.fd(), Poller::Readable);

    client.send("x", 1);

    Poller::Event events[4];
    int n = poller.poll(events, 4, 100);
    ASSERT_GE(n, 1);
    EXPECT_TRUE(events[0].flags & Poller::Readable);

    poller.remove(server.fd());
    client.send("y", 1);
    n = poller.poll(events, 4, 50);
    EXPECT_EQ(n, 0);
}

// -- HangUp on peer close -----------------------------------------------------

TEST(PlatformPoller, DetectsHangUpOnPeerClose) {
    auto [client, server] = make_pair();

    Poller poller;
    poller.add(server, Poller::Readable);

    client.close();

    Poller::Event events[4];
    int n = poller.poll(events, 4, 100);
    ASSERT_GE(n, 1);
    // Should get either Readable (with 0-byte read) or HangUp
    EXPECT_TRUE((events[0].flags & Poller::Readable) || (events[0].flags & Poller::HangUp));
}

// -- Notifier -----------------------------------------------------------------

TEST(PlatformPoller, NotifierWakesPoll) {
    Poller poller;
    int nfd = poller.create_notifier();
    EXPECT_GE(nfd, 0);

    // Poll in a thread — should block until notified
    std::atomic<bool> woke{false};
    std::thread waiter([&] {
        Poller::Event events[4];
        int n = poller.poll(events, 4, 5000); // long timeout
        if (n > 0)
            woke.store(true);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(woke.load()); // should still be blocking

    poller.notify();

    waiter.join();
    EXPECT_TRUE(woke.load());
}

TEST(PlatformPoller, NotifierCanBeCalledMultipleTimes) {
    Poller poller;
    poller.create_notifier();

    // Multiple notifications should not crash or deadlock
    for (int i = 0; i < 10; ++i) {
        poller.notify();
    }

    Poller::Event events[4];
    int n = poller.poll(events, 4, 50);
    EXPECT_GE(n, 1); // at least one notification event
}

// -- Listener socket readability (accept pattern) -----------------------------

TEST(PlatformPoller, MultipleSocketsAllEventsDelivered) {
    // Verify that with two readable sockets, both events are eventually delivered
    // (tests that we don't permanently lose events under load)
    auto [c1, s1] = make_pair();
    auto [c2, s2] = make_pair();

    Poller poller;
    poller.add(s1, Poller::Readable, reinterpret_cast<void*>(1));
    poller.add(s2, Poller::Readable, reinterpret_cast<void*>(2));

    c1.send("a", 1);
    c2.send("b", 1);

    // Drain both sockets' data via polling
    std::set<void*> seen;
    Poller::Event events[4];
    for (int attempt = 0; attempt < 20 && seen.size() < 2; ++attempt) {
        int n = poller.poll(events, 4, 100);
        for (int i = 0; i < n; ++i)
            seen.insert(events[i].user_data);
    }
    EXPECT_EQ(seen.size(), 2u) << "Both socket events should be delivered";
}

TEST(PlatformPoller, NotifierWakeFromAnotherThread) {
    // Verify notifier can wake the poller from a different thread
    Poller poller;
    poller.create_notifier();

    std::atomic<int> wake_count{0};
    std::thread waker([&] {
        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            poller.notify();
        }
    });

    Poller::Event events[4];
    for (int attempt = 0; attempt < 50 && wake_count < 1; ++attempt) {
        int n = poller.poll(events, 4, 200);
        if (n > 0) {
            wake_count++;
            poller.drain_notifier();
        }
    }
    waker.join();
    EXPECT_GE(wake_count.load(), 1) << "Notifier should wake poller from another thread";
}

TEST(PlatformPoller, ListenerReadableOnIncomingConnection) {
    auto listener = tcp_listen("127.0.0.1", 0);
    uint16_t port = listener.local_port();

    listener.set_non_blocking(true);

    Poller poller;
    poller.add(listener, Poller::Readable);

    // No connection yet
    Poller::Event events[4];
    EXPECT_EQ(poller.poll(events, 4, 0), 0);

    // Connect
    std::thread connector([port] { auto s = tcp_connect("127.0.0.1", port); });

    int n = poller.poll(events, 4, 500);
    EXPECT_GE(n, 1);
    EXPECT_TRUE(events[0].flags & Poller::Readable);

    connector.join();
}

// -- New API: recycle_buffer, io_stats, submit_send ---------------------------

TEST(PlatformPoller, RecycleBufferIsNoOpOnReadinessBackend) {
    Poller poller;
    // Should not crash — no-op on epoll/kqueue/WSAPoll
    poller.recycle_buffer(0);
    poller.recycle_buffer(42);
}

TEST(PlatformPoller, IoStatsReturnsZerosOnReadinessBackend) {
    Poller poller;
    auto stats = poller.io_stats();
    EXPECT_EQ(stats.recv_submitted, 0u);
    EXPECT_EQ(stats.recv_completed, 0u);
    EXPECT_EQ(stats.recv_enobufs, 0u);
    EXPECT_EQ(stats.send_submitted, 0u);
    EXPECT_EQ(stats.send_completed, 0u);
    EXPECT_EQ(stats.send_partial, 0u);
    EXPECT_EQ(stats.send_errors, 0u);
    EXPECT_EQ(stats.sqe_full, 0u);
    EXPECT_EQ(stats.cqe_reaped, 0u);
}

TEST(PlatformPoller, SubmitSendReturnsTrueOnReadinessBackend) {
    // On readiness backends, submit_send is synchronous (returns true)
    // and the caller must do the actual send.
    Poller poller;
    const char data[] = "hello";
    size_t bytes = 0;
    bool sync = poller.submit_send(99, data, sizeof(data), &bytes);
    EXPECT_TRUE(sync) << "Readiness backends should return true (synchronous send)";
}

TEST(PlatformPoller, EventFieldsInitializedOnReadiness) {
    // Verify that the new Event fields (data, data_len, buffer_id) are
    // zero-initialized by readiness backends.
    auto [client, server] = make_pair();

    Poller poller;
    poller.add(server, Poller::Readable);
    client.send("x", 1);

    Poller::Event events[4];
    int n = poller.poll(events, 4, 100);
    ASSERT_GE(n, 1);
    EXPECT_EQ(events[0].data, nullptr);
    EXPECT_EQ(events[0].data_len, 0u);
    EXPECT_EQ(events[0].buffer_id, 0u);
}
