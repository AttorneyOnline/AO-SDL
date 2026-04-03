#include <gtest/gtest.h>

#include "net/Http.h"
#include "platform/Socket.h"
#include "utils/Log.h"

#include <atomic>
#include <barrier>
#include <chrono>
#include <latch>
#include <mutex>
#include <numeric>
#include <set>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

// ===========================================================================
// Test fixture: starts server, provides client factory, auto-stops on teardown
// ===========================================================================

class HttpServerTest : public ::testing::Test {
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

// ===========================================================================
// Basic routing
// ===========================================================================

TEST_F(HttpServerTest, RoutesToCorrectHandler) {
    server_.Get("/a", [](const http::Request&, http::Response& res) { res.set_content("handler_a", "text/plain"); });
    server_.Get("/b", [](const http::Request&, http::Response& res) { res.set_content("handler_b", "text/plain"); });
    start();

    auto cli = client();
    auto ra = cli.Get("/a");
    auto rb = cli.Get("/b");
    ASSERT_TRUE(ra);
    ASSERT_TRUE(rb);
    EXPECT_EQ(ra->body, "handler_a");
    EXPECT_EQ(rb->body, "handler_b");
}

TEST_F(HttpServerTest, UnmatchedRouteReturns404) {
    server_.Get("/exists", [](const http::Request&, http::Response& res) { res.set_content("ok", "text/plain"); });
    start();

    auto cli = client();
    auto res = cli.Get("/does-not-exist");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}

TEST_F(HttpServerTest, MethodMismatchReturns404) {
    server_.Get("/only-get", [](const http::Request&, http::Response& res) { res.set_content("ok", "text/plain"); });
    start();

    auto cli = client();
    auto res = cli.Post("/only-get", "", "text/plain");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}

// ===========================================================================
// Handler exceptions
// ===========================================================================

TEST_F(HttpServerTest, HandlerExceptionReturns500) {
    server_.Get("/crash",
                [](const http::Request&, http::Response&) { throw std::runtime_error("deliberate test crash"); });
    start();

    auto cli = client();
    auto res = cli.Get("/crash");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 500);
}

TEST_F(HttpServerTest, ExceptionDoesNotKillServer) {
    server_.Get("/crash", [](const http::Request&, http::Response&) { throw std::runtime_error("boom"); });
    server_.Get("/ok", [](const http::Request&, http::Response& res) { res.set_content("fine", "text/plain"); });
    start();

    auto cli1 = client();
    auto r1 = cli1.Get("/crash");
    ASSERT_TRUE(r1);
    EXPECT_EQ(r1->status, 500);

    // Server should still work
    auto cli2 = client();
    auto r2 = cli2.Get("/ok");
    ASSERT_TRUE(r2);
    EXPECT_EQ(r2->status, 200);
    EXPECT_EQ(r2->body, "fine");
}

// ===========================================================================
// Concurrency: parallel requests
// ===========================================================================

TEST_F(HttpServerTest, ConcurrentRequestsAllServed) {
    std::atomic<int> handler_calls{0};

    server_.Get("/concurrent", [&handler_calls](const http::Request&, http::Response& res) {
        handler_calls.fetch_add(1);
        res.set_content("ok", "text/plain");
    });
    start();

    constexpr int NUM_CLIENTS = 20;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < NUM_CLIENTS; ++i) {
        threads.emplace_back([this, &success_count] {
            auto cli = client();
            auto res = cli.Get("/concurrent");
            if (res && res->status == 200) {
                success_count.fetch_add(1);
            }
        });
    }

    for (auto& t : threads)
        t.join();

    EXPECT_EQ(success_count.load(), NUM_CLIENTS);
    EXPECT_EQ(handler_calls.load(), NUM_CLIENTS);
}

// ===========================================================================
// Concurrency: handler contention with shared mutex
// ===========================================================================

TEST_F(HttpServerTest, ConcurrentRequestsWithSharedMutex) {
    // Simulates the dispatch_mutex_ pattern used by RestRouter.
    // All handlers serialize through a single mutex.
    std::mutex dispatch_mutex;
    std::atomic<int> max_concurrent{0};
    std::atomic<int> current{0};

    server_.Get("/serial", [&](const http::Request&, http::Response& res) {
        std::lock_guard lock(dispatch_mutex);
        int c = current.fetch_add(1) + 1;

        // Track max concurrency — should be 1 under mutex
        int prev = max_concurrent.load();
        while (c > prev && !max_concurrent.compare_exchange_weak(prev, c)) {
        }

        // Simulate brief work
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        current.fetch_sub(1);
        res.set_content("ok", "text/plain");
    });
    start();

    constexpr int N = 10;
    std::vector<std::thread> threads;
    std::atomic<int> ok{0};

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([this, &ok] {
            auto cli = client();
            auto res = cli.Get("/serial");
            if (res && res->status == 200)
                ok.fetch_add(1);
        });
    }

    for (auto& t : threads)
        t.join();

    EXPECT_EQ(ok.load(), N);
    // Under a mutex, max_concurrent should be exactly 1
    EXPECT_EQ(max_concurrent.load(), 1);
}

// ===========================================================================
// Concurrency: slow handler doesn't block other requests
// ===========================================================================

TEST_F(HttpServerTest, SlowHandlerDoesNotBlockFastHandler) {
    std::atomic<bool> slow_started{false};
    std::atomic<bool> fast_done{false};

    server_.Get("/slow", [&](const http::Request&, http::Response& res) {
        slow_started.store(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        res.set_content("slow", "text/plain");
    });
    server_.Get("/fast", [&](const http::Request&, http::Response& res) {
        fast_done.store(true);
        res.set_content("fast", "text/plain");
    });
    start();

    // Start slow request in background
    std::thread slow_thread([this] {
        auto cli = client();
        cli.Get("/slow");
    });

    // Wait for slow handler to enter
    while (!slow_started.load())
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

    // Fast request should complete while slow is still running
    auto cli = client();
    auto res = cli.Get("/fast");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->body, "fast");
    EXPECT_TRUE(fast_done.load());

    slow_thread.join();
}

// ===========================================================================
// Concurrency: rapid connect/disconnect
// ===========================================================================

TEST_F(HttpServerTest, RapidConnectDisconnect) {
    // Clients connecting and immediately closing shouldn't crash the server.
    server_.Get("/test", [](const http::Request&, http::Response& res) { res.set_content("ok", "text/plain"); });
    start();

    for (int i = 0; i < 50; ++i) {
        auto cli = client();
        cli.stop(); // immediately disconnect
    }

    // Server should still work after all the aborted connections
    auto cli = client();
    auto res = cli.Get("/test");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
}

// ===========================================================================
// Concurrency: start/stop lifecycle
// ===========================================================================

TEST_F(HttpServerTest, StopIsIdempotent) {
    server_.Get("/test", [](const http::Request&, http::Response& res) { res.set_content("ok", "text/plain"); });
    start();

    server_.stop();
    server_.stop(); // double stop should not crash
    server_.stop();
}

// ===========================================================================
// Concurrency: thundering herd — all clients fire at the same instant
// ===========================================================================

TEST_F(HttpServerTest, ThunderingHerd) {
    std::atomic<int> served{0};

    server_.Get("/thunder", [&served](const http::Request&, http::Response& res) {
        served.fetch_add(1);
        res.set_content("ok", "text/plain");
    });
    start();

    constexpr int N = 30;
    std::latch gate(1); // all threads wait here, then fire simultaneously
    std::vector<std::thread> threads;
    std::atomic<int> ok{0};

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([this, &gate, &ok] {
            gate.wait(); // block until released
            auto cli = client();
            auto res = cli.Get("/thunder");
            if (res && res->status == 200)
                ok.fetch_add(1);
        });
    }

    // Release all threads at once
    gate.count_down();

    for (auto& t : threads)
        t.join();

    EXPECT_EQ(ok.load(), N);
    EXPECT_EQ(served.load(), N);
}

// ===========================================================================
// Request body parsing
// ===========================================================================

TEST_F(HttpServerTest, PostBodyParsed) {
    std::string received_body;
    server_.Post("/body", [&received_body](const http::Request& req, http::Response& res) {
        received_body = req.body;
        res.set_content("got it", "text/plain");
    });
    start();

    auto cli = client();
    auto res = cli.Post("/body", R"({"key":"value"})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(received_body, R"({"key":"value"})");
}

// ===========================================================================
// Query parameters
// ===========================================================================

TEST_F(HttpServerTest, QueryParametersParsed) {
    std::string captured_q;
    server_.Get("/search", [&captured_q](const http::Request& req, http::Response& res) {
        auto it = req.params.find("q");
        if (it != req.params.end())
            captured_q = it->second;
        res.set_content("ok", "text/plain");
    });
    start();

    auto cli = client();
    auto res = cli.Get("/search?q=hello");
    ASSERT_TRUE(res);
    EXPECT_EQ(captured_q, "hello");
}

// ===========================================================================
// Path parameters via PathParamsMatcher
// ===========================================================================

TEST_F(HttpServerTest, PathParamsMatched) {
    std::string captured_id;
    server_.Get("/users/:id", [&captured_id](const http::Request& req, http::Response& res) {
        auto it = req.path_params.find("id");
        if (it != req.path_params.end())
            captured_id = it->second;
        res.set_content("ok", "text/plain");
    });
    start();

    auto cli = client();
    auto res = cli.Get("/users/42");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(captured_id, "42");
}

// ===========================================================================
// Large response body
// ===========================================================================

TEST_F(HttpServerTest, LargeResponseBody) {
    constexpr size_t SIZE = 1024 * 1024; // 1MB
    server_.Get("/big", [](const http::Request&, http::Response& res) {
        res.set_content(std::string(SIZE, 'X'), "application/octet-stream");
    });
    start();

    auto cli = client();
    auto res = cli.Get("/big");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->body.size(), SIZE);
}

// ===========================================================================
// Concurrent mixed methods
// ===========================================================================

TEST_F(HttpServerTest, ConcurrentMixedMethods) {
    server_.Get("/res", [](const http::Request&, http::Response& res) { res.set_content("get", "text/plain"); });
    server_.Post("/res", [](const http::Request&, http::Response& res) { res.set_content("post", "text/plain"); });
    server_.Put("/res", [](const http::Request&, http::Response& res) { res.set_content("put", "text/plain"); });
    server_.Delete("/res", [](const http::Request&, http::Response& res) { res.set_content("delete", "text/plain"); });
    start();

    std::atomic<int> ok{0};
    std::vector<std::thread> threads;

    auto fire = [&](auto method, const std::string& expected) {
        threads.emplace_back([this, method, &ok, expected] {
            auto cli = client();
            auto res = method(cli);
            if (res && res->body == expected)
                ok.fetch_add(1);
        });
    };

    std::atomic<int> get_ok{0}, post_ok{0}, put_ok{0}, del_ok{0};

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this, &get_ok] {
            auto cli = client();
            auto res = cli.Get("/res");
            if (res && res->body == "get")
                get_ok.fetch_add(1);
        });
        threads.emplace_back([this, &post_ok] {
            auto cli = client();
            auto res = cli.Post("/res", "", "text/plain");
            if (res && res->body == "post")
                post_ok.fetch_add(1);
        });
        threads.emplace_back([this, &put_ok] {
            auto cli = client();
            auto res = cli.Put("/res", "", "text/plain");
            if (res && res->body == "put")
                put_ok.fetch_add(1);
        });
        threads.emplace_back([this, &del_ok] {
            auto cli = client();
            auto res = cli.Delete("/res");
            if (res && res->body == "delete")
                del_ok.fetch_add(1);
        });
    }

    for (auto& t : threads)
        t.join();
    EXPECT_EQ(get_ok.load(), 5) << "GET failures";
    EXPECT_EQ(post_ok.load(), 5) << "POST failures";
    EXPECT_EQ(put_ok.load(), 5) << "PUT failures";
    EXPECT_EQ(del_ok.load(), 5) << "DELETE failures";
}

// ===========================================================================
// Handler thread identity: verify handlers run on worker threads, not poll
// ===========================================================================

TEST_F(HttpServerTest, HandlerRunsOnWorkerThread) {
    auto poll_thread_id = std::this_thread::get_id();
    std::mutex ids_mutex;
    std::set<std::thread::id> handler_thread_ids;

    server_.Get("/thread", [&](const http::Request&, http::Response& res) {
        {
            std::lock_guard lock(ids_mutex);
            handler_thread_ids.insert(std::this_thread::get_id());
        }
        res.set_content("ok", "text/plain");
    });
    start();

    // The server_thread_ is the poll thread
    for (int i = 0; i < 5; ++i) {
        auto cli = client();
        cli.Get("/thread");
    }

    std::lock_guard lock(ids_mutex);
    // Handlers should NOT run on the test thread
    EXPECT_EQ(handler_thread_ids.count(std::this_thread::get_id()), 0u);
    // They should run on at least 1 worker thread (could be more with concurrency)
    EXPECT_GE(handler_thread_ids.size(), 1u);
}

// ===========================================================================
// Body accumulation — the server must wait for Content-Length bytes before
// dispatching, even if the body arrives across multiple TCP segments.
// ===========================================================================

/// Set a receive timeout on a raw socket so tests don't hang on failure.
static void set_recv_timeout(platform::Socket& sock, int sec) {
#ifdef _WIN32
    DWORD tv = sec * 1000;
    setsockopt(sock.fd(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
#else
    struct timeval tv;
    tv.tv_sec = sec;
    tv.tv_usec = 0;
    setsockopt(sock.fd(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
}

/// Read a full HTTP response from a raw socket (until connection close or
/// double-CRLF + Content-Length body).
static std::string read_raw_response(platform::Socket& sock) {
    std::string resp;
    char buf[8192];
    while (true) {
        ssize_t n = sock.recv(buf, sizeof(buf));
        if (n <= 0)
            break;
        resp.append(buf, static_cast<size_t>(n));
    }
    return resp;
}

/// Extract the HTTP status code from a raw response string.
static int extract_status(const std::string& resp) {
    auto sp1 = resp.find(' ');
    if (sp1 == std::string::npos)
        return 0;
    auto sp2 = resp.find(' ', sp1 + 1);
    if (sp2 == std::string::npos)
        sp2 = resp.find('\r', sp1 + 1);
    if (sp2 == std::string::npos)
        return 0;
    return std::stoi(resp.substr(sp1 + 1, sp2 - sp1 - 1));
}

/// Extract the response body (everything after \r\n\r\n).
static std::string extract_body(const std::string& resp) {
    auto pos = resp.find("\r\n\r\n");
    if (pos == std::string::npos)
        return "";
    return resp.substr(pos + 4);
}

// -- Core bug: body arriving in a separate TCP segment -----------------------

TEST_F(HttpServerTest, BodyAccumulation_SplitBodyArrivesComplete) {
    // Regression test: before the fix, the server dispatched the request as
    // soon as headers were complete, causing truncated/empty bodies under
    // concurrency. This test simulates the split by sending headers first,
    // sleeping, then sending the body.
    std::string received_body;
    server_.Post("/echo", [&](const http::Request& req, http::Response& res) {
        received_body = req.body;
        res.set_content(req.body, "text/plain");
    });
    start();

    auto sock = platform::tcp_connect("127.0.0.1", port_);
    set_recv_timeout(sock, 5);

    std::string body = R"({"key":"value","number":42})";
    std::string headers = "POST /echo HTTP/1.1\r\n"
                          "Host: 127.0.0.1\r\n"
                          "Content-Type: application/json\r\n"
                          "Content-Length: " +
                          std::to_string(body.size()) +
                          "\r\n"
                          "Connection: close\r\n"
                          "\r\n";

    // Send headers only — body follows after a delay
    sock.send(headers.data(), headers.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Now send the body
    sock.send(body.data(), body.size());

    auto resp = read_raw_response(sock);
    EXPECT_EQ(extract_status(resp), 200);
    EXPECT_EQ(extract_body(resp), body);
    EXPECT_EQ(received_body, body);
}

TEST_F(HttpServerTest, BodyAccumulation_BodyInMultipleChunks) {
    // Body arrives in three separate TCP writes, each a fragment.
    std::string received_body;
    server_.Post("/echo", [&](const http::Request& req, http::Response& res) {
        received_body = req.body;
        res.set_content(req.body, "text/plain");
    });
    start();

    auto sock = platform::tcp_connect("127.0.0.1", port_);
    set_recv_timeout(sock, 5);

    std::string body = "aaaaaaaaaa"
                       "bbbbbbbbbb"
                       "cccccccccc"; // 30 bytes
    std::string headers = "POST /echo HTTP/1.1\r\n"
                          "Host: 127.0.0.1\r\n"
                          "Content-Length: 30\r\n"
                          "Connection: close\r\n"
                          "\r\n";

    sock.send(headers.data(), headers.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    sock.send(body.data(), 10); // first fragment
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    sock.send(body.data() + 10, 10); // second fragment
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    sock.send(body.data() + 20, 10); // third fragment

    auto resp = read_raw_response(sock);
    EXPECT_EQ(extract_status(resp), 200);
    EXPECT_EQ(received_body, body);
}

TEST_F(HttpServerTest, BodyAccumulation_EmptyBodyWithContentLengthZero) {
    // Content-Length: 0 should dispatch immediately with empty body.
    bool handler_called = false;
    server_.Post("/empty", [&](const http::Request& req, http::Response& res) {
        handler_called = true;
        EXPECT_TRUE(req.body.empty());
        res.set_content("ok", "text/plain");
    });
    start();

    auto sock = platform::tcp_connect("127.0.0.1", port_);
    set_recv_timeout(sock, 5);

    std::string req = "POST /empty HTTP/1.1\r\n"
                      "Host: 127.0.0.1\r\n"
                      "Content-Length: 0\r\n"
                      "Connection: close\r\n"
                      "\r\n";
    sock.send(req.data(), req.size());

    auto resp = read_raw_response(sock);
    EXPECT_EQ(extract_status(resp), 200);
    EXPECT_TRUE(handler_called);
}

TEST_F(HttpServerTest, BodyAccumulation_NoContentLengthMeansEmptyBody) {
    // POST without Content-Length — body is zero per RFC 9112 §6.3 step 7.
    std::string received_body = "sentinel";
    server_.Post("/no-cl", [&](const http::Request& req, http::Response& res) {
        received_body = req.body;
        res.set_content("ok", "text/plain");
    });
    start();

    auto sock = platform::tcp_connect("127.0.0.1", port_);
    set_recv_timeout(sock, 5);

    std::string req = "POST /no-cl HTTP/1.1\r\n"
                      "Host: 127.0.0.1\r\n"
                      "Connection: close\r\n"
                      "\r\n";
    sock.send(req.data(), req.size());

    auto resp = read_raw_response(sock);
    EXPECT_EQ(extract_status(resp), 200);
    EXPECT_EQ(received_body, "");
}

// -- Transfer-Encoding → 411 Length Required ----------------------------------

TEST_F(HttpServerTest, TransferEncoding_Returns411) {
    // Server does not support Transfer-Encoding; should respond 411.
    server_.Post("/te", [](const http::Request&, http::Response& res) { res.set_content("ok", "text/plain"); });
    start();

    auto sock = platform::tcp_connect("127.0.0.1", port_);
    set_recv_timeout(sock, 5);

    std::string req = "POST /te HTTP/1.1\r\n"
                      "Host: 127.0.0.1\r\n"
                      "Transfer-Encoding: chunked\r\n"
                      "\r\n"
                      "0\r\n\r\n";
    sock.send(req.data(), req.size());

    auto resp = read_raw_response(sock);
    EXPECT_EQ(extract_status(resp), 411);
}

// -- Content Too Large → 413 --------------------------------------------------

TEST_F(HttpServerTest, OversizedContentLength_Returns413) {
    server_.Post("/big", [](const http::Request&, http::Response& res) { res.set_content("ok", "text/plain"); });
    start();

    auto sock = platform::tcp_connect("127.0.0.1", port_);
    set_recv_timeout(sock, 5);

    // Claim a 2 GiB body — exceeds MAX_BODY_LENGTH (1 MiB)
    std::string req = "POST /big HTTP/1.1\r\n"
                      "Host: 127.0.0.1\r\n"
                      "Content-Length: 2147483648\r\n"
                      "Connection: close\r\n"
                      "\r\n";
    sock.send(req.data(), req.size());

    auto resp = read_raw_response(sock);
    EXPECT_EQ(extract_status(resp), 413);
}

// -- Peer close before body complete → discard --------------------------------

TEST_F(HttpServerTest, BodyAccumulation_PeerCloseBeforeBodyComplete) {
    // Client sends headers promising 100 bytes but closes after 10.
    // Server should discard the incomplete request, not crash or hang.
    bool handler_called = false;
    server_.Post("/partial", [&](const http::Request&, http::Response& res) {
        handler_called = true;
        res.set_content("ok", "text/plain");
    });
    start();

    auto sock = platform::tcp_connect("127.0.0.1", port_);

    std::string headers = "POST /partial HTTP/1.1\r\n"
                          "Host: 127.0.0.1\r\n"
                          "Content-Length: 100\r\n"
                          "Connection: close\r\n"
                          "\r\n";
    sock.send(headers.data(), headers.size());
    sock.send("short", 5); // Only 5 of 100 bytes
    sock.close();          // Close before body is complete

    // Give server time to process the close event
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Handler should NOT have been called with the incomplete body
    EXPECT_FALSE(handler_called);
}

// -- Keep-alive: second request after body accumulation -----------------------

TEST_F(HttpServerTest, BodyAccumulation_KeepAliveSecondRequest) {
    // After a successful POST with body accumulation, a second request on the
    // same keep-alive connection should also work correctly.
    int call_count = 0;
    server_.Post("/first", [&](const http::Request& req, http::Response& res) {
        ++call_count;
        res.set_content(req.body, "text/plain");
    });
    server_.Get("/second", [&](const http::Request&, http::Response& res) {
        ++call_count;
        res.set_content("ok", "text/plain");
    });
    start();

    auto sock = platform::tcp_connect("127.0.0.1", port_);
    set_recv_timeout(sock, 5);

    // First request: POST with split body
    std::string body1 = "hello world";
    std::string headers1 = "POST /first HTTP/1.1\r\n"
                           "Host: 127.0.0.1\r\n"
                           "Content-Length: " +
                           std::to_string(body1.size()) +
                           "\r\n"
                           "Connection: keep-alive\r\n"
                           "\r\n";
    sock.send(headers1.data(), headers1.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sock.send(body1.data(), body1.size());

    // Read first response
    std::string resp1;
    char buf[8192];
    // Read until we have a complete HTTP response (headers + body)
    while (true) {
        ssize_t n = sock.recv(buf, sizeof(buf));
        if (n <= 0)
            break;
        resp1.append(buf, static_cast<size_t>(n));
        // Check if we have a complete response
        auto hdr_end = resp1.find("\r\n\r\n");
        if (hdr_end != std::string::npos) {
            // Find Content-Length in response
            auto cl_pos = resp1.find("Content-Length: ");
            if (cl_pos != std::string::npos) {
                size_t cl_val = std::stoul(resp1.substr(cl_pos + 16));
                if (resp1.size() >= hdr_end + 4 + cl_val)
                    break;
            }
        }
    }
    EXPECT_EQ(extract_status(resp1), 200);
    EXPECT_EQ(extract_body(resp1), body1);

    // Second request: GET on same connection
    std::string req2 = "GET /second HTTP/1.1\r\n"
                       "Host: 127.0.0.1\r\n"
                       "Connection: close\r\n"
                       "\r\n";
    sock.send(req2.data(), req2.size());

    std::string resp2;
    while (true) {
        ssize_t n = sock.recv(buf, sizeof(buf));
        if (n <= 0)
            break;
        resp2.append(buf, static_cast<size_t>(n));
    }
    EXPECT_EQ(extract_status(resp2), 200);
    EXPECT_EQ(extract_body(resp2), "ok");
    EXPECT_EQ(call_count, 2);
}
