#include <gtest/gtest.h>

#include "platform/Poll.h"
#include "platform/Socket.h"

#include <chrono>
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
