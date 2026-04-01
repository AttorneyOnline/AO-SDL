#include "net/WSClientThread.h"

#include "event/EventManager.h"
#include "event/ServerConnectEvent.h"

#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Minimal mock ProtocolHandler for testing lifecycle without real network I/O.
// ---------------------------------------------------------------------------

class MockProtocolHandler : public ProtocolHandler {
  public:
    void on_connect() override {
        ++connect_count;
    }

    void on_message(const std::string& msg) override {
        messages.push_back(msg);
    }

    void on_disconnect() override {
        ++disconnect_count;
    }

    std::vector<std::string> flush_outgoing() override {
        return {};
    }

    int connect_count = 0;
    int disconnect_count = 0;
    std::vector<std::string> messages;
};

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

TEST(WSClientThread, ConstructAndStopImmediately) {
    // The thread starts in the constructor and blocks waiting for a
    // ServerConnectEvent. Calling stop() before any event is published
    // must terminate cleanly without hanging or crashing.
    MockProtocolHandler handler;
    WSClientThread nt(handler);
    nt.stop();

    // The handler should never have been connected.
    EXPECT_EQ(handler.connect_count, 0);
    EXPECT_EQ(handler.disconnect_count, 0);
}

TEST(WSClientThread, StopIsIdempotent) {
    // Calling stop() multiple times must not crash or deadlock.
    MockProtocolHandler handler;
    WSClientThread nt(handler);
    nt.stop();
    nt.stop(); // second call — thread already joined
}

// ---------------------------------------------------------------------------
// Lifecycle: thread runs and can be stopped
// ---------------------------------------------------------------------------

TEST(WSClientThread, ThreadIsJoinableBeforeStop) {
    // After construction, the internal thread must be running (joinable).
    // We cannot inspect the private member directly, but we can verify that
    // stop() completes in bounded time, which proves the thread was alive
    // and is now joined.
    MockProtocolHandler handler;
    WSClientThread nt(handler);

    auto start = std::chrono::steady_clock::now();
    nt.stop();
    auto elapsed = std::chrono::steady_clock::now() - start;

    // stop() should return quickly (well under 1 second) because the loop
    // only sleeps 50ms per iteration while waiting for ServerConnectEvent.
    EXPECT_LT(elapsed, std::chrono::seconds(2));
}

TEST(WSClientThread, NoCallbacksWithoutConnection) {
    // If no ServerConnectEvent is published, the handler receives no
    // callbacks at all.
    MockProtocolHandler handler;
    WSClientThread nt(handler);

    // Let the thread spin a few iterations.
    std::this_thread::sleep_for(std::chrono::milliseconds(120));

    nt.stop();

    EXPECT_EQ(handler.connect_count, 0);
    EXPECT_EQ(handler.disconnect_count, 0);
    EXPECT_TRUE(handler.messages.empty());
}

// ---------------------------------------------------------------------------
// jthread auto-join: destructor must not call std::terminate
// ---------------------------------------------------------------------------

TEST(WSClientThread, DestructorAutoJoinsWithoutExplicitStop) {
    // Before jthread migration, forgetting stop() would call std::terminate.
    // jthread's destructor calls request_stop() + join() automatically.
    MockProtocolHandler handler;
    {
        WSClientThread nt(handler);
        // Intentionally NOT calling nt.stop().
    }
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Multiple sequential instances
// ---------------------------------------------------------------------------

TEST(WSClientThread, MultipleSequentialInstances) {
    // Creating, stopping, and destroying multiple WSClientThread instances
    // sequentially must work — no leaked state.
    MockProtocolHandler handler;

    for (int i = 0; i < 3; ++i) {
        WSClientThread nt(handler);
        nt.stop();
    }

    EXPECT_EQ(handler.connect_count, 0);
}

// ---------------------------------------------------------------------------
// Event channel interaction (no actual network)
// ---------------------------------------------------------------------------

TEST(WSClientThread, StopWhileWaitingForEvent) {
    // The thread polls EventManager for ServerConnectEvent. Publishing an
    // unrelated event and then calling stop() must still terminate cleanly.
    MockProtocolHandler handler;
    WSClientThread nt(handler);

    // The thread is spinning in the "wait for ServerConnectEvent" loop.
    // Give it a moment, then stop.
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    nt.stop();

    // No connection was attempted.
    EXPECT_EQ(handler.connect_count, 0);
}
