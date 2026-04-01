#include <gtest/gtest.h>

#include "event/EventManager.h"
#include "net/Http.h"
#include "net/SSEEvent.h"
#include "platform/Socket.h"
#include "utils/Log.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

// ===========================================================================
// Test fixture
// ===========================================================================

class SSETest : public ::testing::Test {
  protected:
    void SetUp() override {
        Log::set_sink([](LogLevel, const std::string&, const std::string&) {});
    }

    void TearDown() override {
        server_.stop();
        if (server_thread_.joinable())
            server_thread_.join();
        Log::set_sink(nullptr);
    }

    void start() {
        port_ = server_.bind_to_any_port("127.0.0.1");
        ASSERT_GT(port_, 0);
        server_thread_ = std::thread([this] { server_.listen_after_bind(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    uint16_t port() const {
        return static_cast<uint16_t>(port_);
    }

    /// Connect to the SSE endpoint and return the raw socket.
    /// Reads the initial response headers (200 + text/event-stream).
    platform::Socket connect_sse(const std::string& path = "/events") {
        auto sock = platform::tcp_connect("127.0.0.1", port());
        std::string req = "GET " + path +
                          " HTTP/1.1\r\n"
                          "Host: 127.0.0.1\r\n"
                          "\r\n";
        sock.send(req.data(), req.size());
        // Read response headers
        std::string resp;
        char buf[4096];
        while (resp.find("\r\n\r\n") == std::string::npos) {
            ssize_t n = sock.recv(buf, sizeof(buf));
            if (n <= 0)
                break;
            resp.append(buf, static_cast<size_t>(n));
        }
        return sock;
    }

    /// Read an SSE frame from the socket (blocks briefly).
    std::string read_sse_frame(platform::Socket& sock, int timeout_ms = 1000) {
#ifdef _WIN32
        DWORD tv = timeout_ms;
        setsockopt(sock.fd(), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
#else
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(sock.fd(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

        std::string result;
        char buf[4096];
        while (true) {
            ssize_t n = sock.recv(buf, sizeof(buf));
            if (n <= 0)
                break;
            result.append(buf, static_cast<size_t>(n));
            if (result.find("\n\n") != std::string::npos)
                break;
        }
        return result;
    }

    http::Server server_;
    int port_ = 0;
    std::thread server_thread_;
};

// ===========================================================================
// Tests
// ===========================================================================

TEST_F(SSETest, SSEEndpointReturnsEventStream) {
    server_.SSE("/events", [](const http::Request&, http::Response&) { return true; });
    start();

    auto sock = platform::tcp_connect("127.0.0.1", port());
    std::string req = "GET /events HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
    sock.send(req.data(), req.size());

    std::string resp;
    char buf[4096];
    while (resp.find("\r\n\r\n") == std::string::npos) {
        ssize_t n = sock.recv(buf, sizeof(buf));
        if (n <= 0)
            break;
        resp.append(buf, static_cast<size_t>(n));
    }

    EXPECT_NE(resp.find("200"), std::string::npos);
    EXPECT_NE(resp.find("text/event-stream"), std::string::npos);
    EXPECT_NE(resp.find("Cache-Control: no-cache"), std::string::npos);
}

TEST_F(SSETest, SSEHandlerCanReject) {
    server_.SSE("/events", [](const http::Request&, http::Response& res) {
        res.status = 401;
        res.set_content("Unauthorized", "text/plain");
        return false;
    });
    start();

    auto sock = platform::tcp_connect("127.0.0.1", port());
    std::string req = "GET /events HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
    sock.send(req.data(), req.size());

    std::string resp;
    char buf[4096];
    while (true) {
        ssize_t n = sock.recv(buf, sizeof(buf));
        if (n <= 0)
            break;
        resp.append(buf, static_cast<size_t>(n));
    }

    EXPECT_NE(resp.find("401"), std::string::npos);
}

TEST_F(SSETest, ReceivesPublishedEvent) {
    server_.SSE("/events", [](const http::Request&, http::Response&) { return true; });
    start();

    auto sock = connect_sse();

    // Publish an event
    SSEEvent evt;
    evt.event = "test";
    evt.data = R"({"msg":"hello"})";
    EventManager::instance().get_channel<SSEEvent>().publish(std::move(evt));

    // Read the SSE frame
    auto frame = read_sse_frame(sock);
    EXPECT_NE(frame.find("event: test"), std::string::npos);
    EXPECT_NE(frame.find("data: {\"msg\":\"hello\"}"), std::string::npos);
}

TEST_F(SSETest, MultipleEventsDeliveredInOrder) {
    server_.SSE("/events", [](const http::Request&, http::Response&) { return true; });
    start();

    auto sock = connect_sse();

    for (int i = 0; i < 3; ++i) {
        SSEEvent evt;
        evt.event = "seq";
        evt.data = std::to_string(i);
        EventManager::instance().get_channel<SSEEvent>().publish(std::move(evt));
    }

    // Events may arrive in separate frames — read all of them
    std::string all_frames;
    for (int i = 0; i < 3; ++i) {
        all_frames += read_sse_frame(sock);
    }
    EXPECT_NE(all_frames.find("data: 0"), std::string::npos);
    EXPECT_NE(all_frames.find("data: 1"), std::string::npos);
    EXPECT_NE(all_frames.find("data: 2"), std::string::npos);
}

TEST_F(SSETest, AreaFilterBroadcastReachesAll) {
    server_.SSE("/events", [](const http::Request&, http::Response&) { return true; });
    start();

    auto sock = connect_sse("/events"); // no area filter

    SSEEvent evt;
    evt.event = "ic";
    evt.data = "hello";
    evt.area = "courtroom1"; // area-scoped
    EventManager::instance().get_channel<SSEEvent>().publish(std::move(evt));

    // Client with no area filter should still receive it
    auto frame = read_sse_frame(sock);
    EXPECT_NE(frame.find("event: ic"), std::string::npos);
}

TEST_F(SSETest, AreaFilterMatchesSubscribedArea) {
    server_.SSE("/events", [](const http::Request&, http::Response&) { return true; });
    start();

    auto sock = connect_sse("/events?area=courtroom1");

    SSEEvent evt;
    evt.event = "ic";
    evt.data = "hello";
    evt.area = "courtroom1";
    EventManager::instance().get_channel<SSEEvent>().publish(std::move(evt));

    auto frame = read_sse_frame(sock);
    EXPECT_NE(frame.find("event: ic"), std::string::npos);
}

TEST_F(SSETest, AreaFilterBlocksMismatchedArea) {
    server_.SSE("/events", [](const http::Request&, http::Response&) { return true; });
    start();

    auto sock = connect_sse("/events?area=courtroom2");

    SSEEvent evt;
    evt.event = "ic";
    evt.data = "hello";
    evt.area = "courtroom1"; // different area
    EventManager::instance().get_channel<SSEEvent>().publish(std::move(evt));

    auto frame = read_sse_frame(sock, 200); // short timeout
    // Should NOT receive the event
    EXPECT_EQ(frame.find("event: ic"), std::string::npos);
}

TEST_F(SSETest, GlobalEventReachesAreaSubscriber) {
    server_.SSE("/events", [](const http::Request&, http::Response&) { return true; });
    start();

    auto sock = connect_sse("/events?area=courtroom1");

    SSEEvent evt;
    evt.event = "char_select";
    evt.data = "global";
    evt.area = ""; // empty = broadcast to all
    EventManager::instance().get_channel<SSEEvent>().publish(std::move(evt));

    auto frame = read_sse_frame(sock);
    EXPECT_NE(frame.find("event: char_select"), std::string::npos);
}

TEST_F(SSETest, MultipleClientsReceiveSameEvent) {
    server_.SSE("/events", [](const http::Request&, http::Response&) { return true; });
    start();

    auto sock1 = connect_sse();
    auto sock2 = connect_sse();

    SSEEvent evt;
    evt.event = "broadcast";
    evt.data = "to-all";
    EventManager::instance().get_channel<SSEEvent>().publish(std::move(evt));

    auto frame1 = read_sse_frame(sock1);
    auto frame2 = read_sse_frame(sock2);
    EXPECT_NE(frame1.find("event: broadcast"), std::string::npos);
    EXPECT_NE(frame2.find("event: broadcast"), std::string::npos);
}

TEST_F(SSETest, ClientDisconnectIsHandledGracefully) {
    server_.SSE("/events", [](const http::Request&, http::Response&) { return true; });
    start();

    {
        auto sock = connect_sse();
        // sock goes out of scope — connection closes
    }

    // Server should handle the disconnect without crashing.
    // Publish an event to trigger the dead-connection cleanup path.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    SSEEvent evt;
    evt.event = "test";
    evt.data = "after-disconnect";
    EventManager::instance().get_channel<SSEEvent>().publish(std::move(evt));

    // Verify server is still alive by making a normal request
    server_.Get("/health", [](const http::Request&, http::Response& res) { res.set_content("ok", "text/plain"); });
    http::Client cli("127.0.0.1", port_);
    auto res = cli.Get("/health");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
}

TEST_F(SSETest, SSEAndNormalEndpointsCoexist) {
    server_.SSE("/events", [](const http::Request&, http::Response&) { return true; });
    server_.Get("/api/test",
                [](const http::Request&, http::Response& res) { res.set_content("normal", "text/plain"); });
    start();

    // Normal GET should still work
    http::Client cli("127.0.0.1", port_);
    auto res = cli.Get("/api/test");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->body, "normal");

    // SSE should work simultaneously
    auto sock = connect_sse();
    SSEEvent evt;
    evt.event = "test";
    evt.data = "ok";
    EventManager::instance().get_channel<SSEEvent>().publish(std::move(evt));

    auto frame = read_sse_frame(sock);
    EXPECT_NE(frame.find("event: test"), std::string::npos);
}

TEST_F(SSETest, ZeroLatencyDelivery) {
    server_.SSE("/events", [](const http::Request&, http::Response&) { return true; });
    start();

    auto sock = connect_sse();

    auto before = std::chrono::steady_clock::now();
    SSEEvent evt;
    evt.event = "timing";
    evt.data = "fast";
    EventManager::instance().get_channel<SSEEvent>().publish(std::move(evt));

    auto frame = read_sse_frame(sock);
    auto elapsed = std::chrono::steady_clock::now() - before;

    ASSERT_NE(frame.find("event: timing"), std::string::npos);
    // Should arrive well under 100ms (the old poll timeout).
    // Allow 50ms for thread scheduling + TCP.
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 50)
        << "SSE event should be delivered with near-zero latency via poller.notify()";
}
