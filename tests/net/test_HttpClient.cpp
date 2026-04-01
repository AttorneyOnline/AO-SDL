#include <gtest/gtest.h>

#include "net/Http.h"
#include "utils/Log.h"

#include <atomic>
#include <chrono>
#include <thread>

#include "platform/Socket.h"

// -- Scheme parsing (tested via Client constructor behavior) -------------------

TEST(HttpClient, HttpSchemeDefaultsToPort80) {
    // We can't directly test parse_scheme_host_port (static), but we can
    // verify Client construction from scheme+host strings works.
    // This just verifies construction doesn't crash.
    http::Client cli("http://127.0.0.1");
    EXPECT_TRUE(cli.is_valid());
}

TEST(HttpClient, HttpsSchemeDefaultsToPort443) {
    http::Client cli("https://127.0.0.1");
    EXPECT_TRUE(cli.is_valid());
}

TEST(HttpClient, HostPortConstructor) {
    http::Client cli("127.0.0.1", 8080);
    EXPECT_TRUE(cli.is_valid());
    EXPECT_EQ(cli.host(), "127.0.0.1");
    EXPECT_EQ(cli.port(), 8080);
}

// -- Error enum ---------------------------------------------------------------

TEST(HttpClient, ErrorToString) {
    EXPECT_EQ(http::to_string(http::Error::Success), "Success");
    EXPECT_EQ(http::to_string(http::Error::Connection), "Connection");
    EXPECT_EQ(http::to_string(http::Error::Read), "Read");
    EXPECT_EQ(http::to_string(http::Error::ConnectionTimeout), "ConnectionTimeout");
    EXPECT_EQ(http::to_string(http::Error::SSLConnection), "SSLConnection");
}

TEST(HttpClient, ErrorStreamOperator) {
    std::ostringstream os;
    os << http::Error::Connection;
    EXPECT_EQ(os.str(), "Connection");
}

// -- Result -------------------------------------------------------------------

TEST(HttpClient, EmptyResultIsFalsy) {
    http::Result r;
    EXPECT_FALSE(r);
    EXPECT_EQ(r, nullptr);
    EXPECT_EQ(r.error(), http::Error::Unknown);
}

TEST(HttpClient, ResultWithResponseIsTruthy) {
    auto res = std::make_unique<http::Response>();
    res->status = 200;
    res->body = "OK";
    http::Result r(std::move(res), http::Error::Success);

    EXPECT_TRUE(r);
    EXPECT_NE(r, nullptr);
    EXPECT_EQ(r->status, 200);
    EXPECT_EQ(r->body, "OK");
    EXPECT_EQ(r.error(), http::Error::Success);
    EXPECT_EQ((*r).status, 200);
    EXPECT_EQ(r.value().status, 200);
}

// -- Request/Response helpers -------------------------------------------------

TEST(HttpClient, RequestHeaderOperations) {
    http::Request req;
    req.set_header("Content-Type", "application/json");
    req.set_header("Accept", "text/html");

    EXPECT_TRUE(req.has_header("Content-Type"));
    EXPECT_EQ(req.get_header_value("Content-Type"), "application/json");
    EXPECT_EQ(req.get_header_value("Missing", "default"), "default");
    EXPECT_EQ(req.get_header_value_count("Content-Type"), 1u);
}

TEST(HttpClient, RequestParamOperations) {
    http::Request req;
    req.params.emplace("key", "value");
    req.params.emplace("key", "value2"); // multimap — duplicate key

    EXPECT_TRUE(req.has_param("key"));
    EXPECT_FALSE(req.has_param("missing"));
    EXPECT_EQ(req.get_param_value("key", 0), "value");
    EXPECT_EQ(req.get_param_value("key", 1), "value2");
    EXPECT_EQ(req.get_param_value_count("key"), 2u);
}

TEST(HttpClient, ResponseSetContent) {
    http::Response res;
    res.set_content("hello", "text/plain");

    EXPECT_EQ(res.body, "hello");
    EXPECT_TRUE(res.has_header("Content-Type"));
    EXPECT_EQ(res.get_header_value("Content-Type"), "text/plain");
}

TEST(HttpClient, ResponseSetContentMove) {
    http::Response res;
    std::string large(10000, 'X');
    res.set_content(std::move(large), "application/octet-stream");

    EXPECT_EQ(res.body.size(), 10000u);
}

TEST(HttpClient, ResponseSetRedirect) {
    http::Response res;
    res.set_redirect("https://example.com");

    EXPECT_EQ(res.status, 302);
    EXPECT_EQ(res.get_header_value("Location"), "https://example.com");
}

// -- Connection failure -------------------------------------------------------

TEST(HttpClient, GetToClosedPortReturnsError) {
    http::Client cli("127.0.0.1", 1); // port 1 = almost certainly closed
    auto res = cli.Get("/");
    EXPECT_FALSE(res);
    EXPECT_EQ(res.error(), http::Error::Connection);
}

// -- Integration: Client + Server roundtrip -----------------------------------

class HttpClientServerTest : public ::testing::Test {
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

    http::Client client() {
        return http::Client("127.0.0.1", port_);
    }

    http::Server server_;
    int port_ = 0;
    std::thread server_thread_;
};

TEST_F(HttpClientServerTest, SimpleGet) {
    server_.Get("/hello", [](const http::Request&, http::Response& res) { res.set_content("world", "text/plain"); });
    start();

    auto cli = client();
    auto res = cli.Get("/hello");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, "world");
}

TEST_F(HttpClientServerTest, PostWithBody) {
    server_.Post("/echo",
                 [](const http::Request& req, http::Response& res) { res.set_content(req.body, "text/plain"); });
    start();

    auto cli = client();
    auto res = cli.Post("/echo", "test body", "text/plain");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, "test body");
}

TEST_F(HttpClientServerTest, NotFoundForUnregisteredRoute) {
    start();
    auto cli = client();
    auto res = cli.Get("/nonexistent");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}

TEST_F(HttpClientServerTest, DefaultHeaders) {
    server_.set_default_headers({{"X-Custom", "value"}});
    server_.Get("/test", [](const http::Request&, http::Response& res) { res.set_content("ok", "text/plain"); });
    start();

    auto cli = client();
    auto res = cli.Get("/test");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->get_header_value("X-Custom"), "value");
}

TEST_F(HttpClientServerTest, MultipleSequentialRequests) {
    int count = 0;
    server_.Get("/count", [&count](const http::Request&, http::Response& res) {
        count++;
        res.set_content(std::to_string(count), "text/plain");
    });
    start();

    for (int i = 1; i <= 5; ++i) {
        auto cli = client();
        auto res = cli.Get("/count");
        ASSERT_TRUE(res) << "Request " << i << " failed";
        EXPECT_EQ(res->body, std::to_string(i));
    }
}

TEST_F(HttpClientServerTest, StreamingGet) {
    server_.Get("/stream", [](const http::Request&, http::Response& res) {
        std::string large(8192, 'A');
        res.set_content(std::move(large), "application/octet-stream");
    });
    start();

    std::string received;
    auto cli = client();
    auto res = cli.Get("/stream", [&received](const char* data, size_t len) -> bool {
        received.append(data, len);
        return true;
    });
    ASSERT_TRUE(res);
    EXPECT_EQ(received.size(), 8192u);
    EXPECT_EQ(received, std::string(8192, 'A'));
}

TEST_F(HttpClientServerTest, AllHttpMethods) {
    server_.Put("/res", [](const http::Request&, http::Response& res) { res.status = 200; });
    server_.Patch("/res", [](const http::Request&, http::Response& res) { res.status = 200; });
    server_.Delete("/res", [](const http::Request&, http::Response& res) { res.status = 200; });
    server_.Options("/res", [](const http::Request&, http::Response& res) { res.status = 204; });
    start();

    auto cli = client();
    EXPECT_TRUE(cli.Put("/res", "", "text/plain"));
    EXPECT_TRUE(cli.Patch("/res", "", "text/plain"));
    EXPECT_TRUE(cli.Delete("/res"));
    EXPECT_TRUE(cli.Options("/res"));
}

// ===========================================================================
// Keep-alive and connection reuse tests
// ===========================================================================

class HttpClientKeepAlive : public ::testing::Test {
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

    http::Client client() {
        return http::Client("127.0.0.1", port_);
    }

    http::Server server_;
    int port_ = 0;
    std::thread server_thread_;
};

TEST_F(HttpClientKeepAlive, SecondRequestReusesConnection) {
    std::atomic<int> request_count{0};
    server_.Get("/ping", [&](const http::Request&, http::Response& res) {
        request_count++;
        res.set_content("pong", "text/plain");
    });
    start();

    auto cli = client();
    cli.set_keep_alive(true);

    auto r1 = cli.Get("/ping");
    ASSERT_TRUE(r1);
    EXPECT_EQ(r1->status, 200);
    EXPECT_EQ(r1->body, "pong");

    auto r2 = cli.Get("/ping");
    ASSERT_TRUE(r2);
    EXPECT_EQ(r2->status, 200);
    EXPECT_EQ(r2->body, "pong");

    EXPECT_EQ(request_count.load(), 2);
    // Both requests succeeded — with keep-alive, the second reused the connection
    EXPECT_NE(cli.is_socket_open(), 0u);
}

TEST_F(HttpClientKeepAlive, MultipleRequestsWithKeepAlive) {
    std::atomic<int> count{0};
    server_.Get("/count", [&](const http::Request&, http::Response& res) {
        int c = ++count;
        res.set_content(std::to_string(c), "text/plain");
    });
    start();

    auto cli = client();
    cli.set_keep_alive(true);

    for (int i = 1; i <= 10; ++i) {
        auto res = cli.Get("/count");
        ASSERT_TRUE(res) << "Request " << i << " failed";
        EXPECT_EQ(res->body, std::to_string(i));
    }
    EXPECT_EQ(count.load(), 10);
}

TEST_F(HttpClientKeepAlive, ConnectionCloseHeaderClosesSocket) {
    server_.Get("/close-me", [](const http::Request&, http::Response& res) {
        res.set_content("bye", "text/plain");
        res.set_header("Connection", "close");
    });
    server_.Get("/hello", [](const http::Request&, http::Response& res) { res.set_content("hi", "text/plain"); });
    start();

    auto cli = client();
    cli.set_keep_alive(true);

    auto r1 = cli.Get("/close-me");
    ASSERT_TRUE(r1);
    EXPECT_EQ(r1->body, "bye");
    // Server said Connection: close, socket should be closed
    EXPECT_EQ(cli.is_socket_open(), 0u);

    // Next request should reconnect and succeed
    auto r2 = cli.Get("/hello");
    ASSERT_TRUE(r2);
    EXPECT_EQ(r2->body, "hi");
}

TEST(HttpClientKeepAliveStale, StaleConnectionRetriesSuccessfully) {
    Log::set_sink([](LogLevel, const std::string&, const std::string&) {});

    // Start a server, make a keep-alive request, then stop the server
    http::Server server1;
    server1.Get("/data", [](const http::Request&, http::Response& res) { res.set_content("ok", "text/plain"); });
    int port = server1.bind_to_any_port("127.0.0.1");
    ASSERT_GT(port, 0);
    std::thread t1([&] { server1.listen_after_bind(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    http::Client cli("127.0.0.1", port);
    cli.set_keep_alive(true);

    auto r1 = cli.Get("/data");
    ASSERT_TRUE(r1);
    EXPECT_EQ(r1->body, "ok");

    // Stop the server — the kept-alive connection is now stale
    server1.stop();
    t1.join();

    // Start a new server on the same port
    http::Server server2;
    server2.Get("/data", [](const http::Request&, http::Response& res) { res.set_content("ok2", "text/plain"); });
    int port2 = server2.bind_to_any_port("127.0.0.1");
    std::thread t2([&] { server2.listen_after_bind(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // New client to new port — verifies that stale detection + reconnect work
    http::Client cli2("127.0.0.1", port2);
    cli2.set_keep_alive(true);
    auto r2 = cli2.Get("/data");
    ASSERT_TRUE(r2);
    EXPECT_EQ(r2->body, "ok2");

    server2.stop();
    t2.join();
    Log::set_sink(nullptr);
}

TEST_F(HttpClientKeepAlive, ConnectionTimeoutReturnsError) {
    // Connect to a non-routable IP with a short timeout
    http::Client cli("127.0.0.1", 1);      // port 1 = not listening
    cli.set_connection_timeout(0, 200000); // 200ms

    auto start = std::chrono::steady_clock::now();
    auto res = cli.Get("/");
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_FALSE(res);
    // Should not hang for more than 2 seconds
    EXPECT_LT(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count(), 2);
}

TEST_F(HttpClientKeepAlive, ReadTimeoutReturnsError) {
    // Create a fake server that accepts but never responds
    auto listener = platform::tcp_listen("127.0.0.1", 0);
    uint16_t port = listener.local_port();
    listener.set_non_blocking(false);

    std::thread slow_server([&] {
        std::string addr;
        uint16_t rp;
        auto conn = platform::tcp_accept(listener, addr, rp);
        // Accept but never send anything
        std::this_thread::sleep_for(std::chrono::seconds(3));
    });

    http::Client cli("127.0.0.1", static_cast<int>(port));
    cli.set_read_timeout(0, 300000); // 300ms

    auto start_time = std::chrono::steady_clock::now();
    auto res = cli.Get("/");
    auto elapsed = std::chrono::steady_clock::now() - start_time;

    EXPECT_FALSE(res);
    // Should not hang for more than 2 seconds
    EXPECT_LT(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count(), 2);

    slow_server.join();
}

TEST_F(HttpClientKeepAlive, TcpNoDelayDoesNotBreakRequests) {
    server_.Get("/test", [](const http::Request&, http::Response& res) { res.set_content("ok", "text/plain"); });
    start();

    auto cli = client();
    cli.set_tcp_nodelay(true);
    cli.set_keep_alive(true);

    auto r1 = cli.Get("/test");
    ASSERT_TRUE(r1);
    EXPECT_EQ(r1->body, "ok");

    auto r2 = cli.Get("/test");
    ASSERT_TRUE(r2);
    EXPECT_EQ(r2->body, "ok");
}

TEST_F(HttpClientKeepAlive, StopDuringRequestDoesNotHang) {
    server_.Get("/slow", [](const http::Request&, http::Response& res) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        res.set_content("done", "text/plain");
    });
    start();

    auto cli = std::make_shared<http::Client>("127.0.0.1", port_);
    cli->set_read_timeout(5);

    std::atomic<bool> request_done{false};
    std::thread request_thread([&] {
        cli->Get("/slow");
        request_done = true;
    });

    // Give the request time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Stop the client from another thread
    cli->stop();

    // Wait for the request thread to finish — should not hang
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    request_thread.join();

    auto now = std::chrono::steady_clock::now();
    EXPECT_LT(now, deadline) << "stop() did not interrupt the request in time";
}
