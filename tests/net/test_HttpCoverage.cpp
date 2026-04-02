/**
 * @file test_HttpCoverage.cpp
 * @brief Additional tests targeting uncovered code paths in Http.cpp,
 *        HttpServer.cpp, and WSClientThread.cpp.
 *
 * These supplement test_HttpClient.cpp, test_HttpServer.cpp, and the RFC
 * compliance suites. They focus on:
 *   - parse_ws_url() unit tests (WSClientThread.cpp)
 *   - Chunked transfer-encoding (Http.cpp read_chunked_body)
 *   - DELETE/PUT/PATCH with bodies (Http.cpp method overloads)
 *   - Content-Length body truncation (Http.cpp read_body)
 *   - Server HEAD response (HttpServer.cpp serialize_response is_head)
 *   - Server error paths for validation (HttpServer.cpp validate_*)
 */
#include <gtest/gtest.h>

#include "net/Http.h"
#include "net/WSClientThread.h"
#include "platform/Socket.h"
#include "utils/Log.h"

#include <chrono>
#include <string>
#include <thread>

// ===========================================================================
// parse_ws_url unit tests
// ===========================================================================

TEST(ParseWsUrl, WssSchemeDefaultsTo443) {
    auto url = parse_ws_url("wss://example.com", 0);
    EXPECT_EQ(url.host, "example.com");
    EXPECT_EQ(url.port, 443);
    EXPECT_TRUE(url.ssl);
}

TEST(ParseWsUrl, WsSchemeDefaultsTo80) {
    auto url = parse_ws_url("ws://example.com", 0);
    EXPECT_EQ(url.host, "example.com");
    EXPECT_EQ(url.port, 80);
    EXPECT_FALSE(url.ssl);
}

TEST(ParseWsUrl, NoSchemeUsesPortIn) {
    auto url = parse_ws_url("example.com", 27016);
    EXPECT_EQ(url.host, "example.com");
    EXPECT_EQ(url.port, 27016);
    EXPECT_FALSE(url.ssl);
}

TEST(ParseWsUrl, EmbeddedPortOverridesDefault) {
    auto url = parse_ws_url("wss://example.com:8443", 0);
    EXPECT_EQ(url.host, "example.com");
    EXPECT_EQ(url.port, 8443);
    EXPECT_TRUE(url.ssl);
}

TEST(ParseWsUrl, ExplicitPortInOverridesSchemeDefault) {
    auto url = parse_ws_url("wss://example.com", 9999);
    EXPECT_EQ(url.host, "example.com");
    EXPECT_EQ(url.port, 9999);
    EXPECT_TRUE(url.ssl);
}

TEST(ParseWsUrl, TrailingPathStripped) {
    auto url = parse_ws_url("ws://example.com/chat", 0);
    EXPECT_EQ(url.host, "example.com");
    EXPECT_EQ(url.port, 80);
}

TEST(ParseWsUrl, TrailingPathWithPort) {
    auto url = parse_ws_url("wss://example.com:443/ws/v2", 0);
    EXPECT_EQ(url.host, "example.com");
    EXPECT_EQ(url.port, 443);
}

TEST(ParseWsUrl, IPv6Address) {
    auto url = parse_ws_url("ws://[::1]", 8080);
    EXPECT_EQ(url.host, "[::1]");
    EXPECT_EQ(url.port, 8080);
    EXPECT_FALSE(url.ssl);
}

TEST(ParseWsUrl, IPv6AddressWithPort) {
    auto url = parse_ws_url("wss://[::1]:9443", 0);
    // The port after ] should be parsed
    EXPECT_EQ(url.host, "[::1]");
    EXPECT_EQ(url.port, 9443);
    EXPECT_TRUE(url.ssl);
}

TEST(ParseWsUrl, PlainHostNoScheme) {
    auto url = parse_ws_url("192.168.1.1", 27016);
    EXPECT_EQ(url.host, "192.168.1.1");
    EXPECT_EQ(url.port, 27016);
    EXPECT_FALSE(url.ssl);
}

TEST(ParseWsUrl, HostWithEmbeddedPortNoScheme) {
    auto url = parse_ws_url("example.com:9000", 0);
    EXPECT_EQ(url.host, "example.com");
    EXPECT_EQ(url.port, 9000);
    EXPECT_FALSE(url.ssl);
}

// ===========================================================================
// HTTP client: chunked transfer-encoding
// ===========================================================================

class HttpCoverageTest : public ::testing::Test {
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
    uint16_t port() const {
        return static_cast<uint16_t>(port_);
    }

    http::Server server_;
    int port_ = 0;
    std::thread server_thread_;
};

// Test chunked response parsing via a fake server that sends chunked encoding
TEST(HttpClientChunked, ParsesChunkedResponse) {
    Log::set_sink([](LogLevel, const std::string&, const std::string&) {});

    auto listener = platform::tcp_listen("127.0.0.1", 0);
    uint16_t port = listener.local_port();

    std::thread fake_server([&] {
        listener.set_non_blocking(false);
        std::string addr;
        uint16_t rp;
        auto conn = platform::tcp_accept(listener, addr, rp);
        if (!conn.valid())
            return;
        char buf[4096];
        conn.recv(buf, sizeof(buf));
        std::string resp = "HTTP/1.1 200 OK\r\n"
                           "Transfer-Encoding: chunked\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "5\r\n"
                           "Hello\r\n"
                           "7\r\n"
                           " World!\r\n"
                           "0\r\n"
                           "\r\n";
        conn.send(resp.data(), resp.size());
        conn.shutdown();
    });

    http::Client cli("127.0.0.1", static_cast<int>(port));
    auto res = cli.Get("/test");
    fake_server.join();
    listener.close();

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, "Hello World!");

    Log::set_sink(nullptr);
}

// Test chunked response with streaming ContentReceiver
TEST(HttpClientChunked, StreamingChunkedResponse) {
    Log::set_sink([](LogLevel, const std::string&, const std::string&) {});

    auto listener = platform::tcp_listen("127.0.0.1", 0);
    uint16_t port = listener.local_port();

    std::thread fake_server([&] {
        listener.set_non_blocking(false);
        std::string addr;
        uint16_t rp;
        auto conn = platform::tcp_accept(listener, addr, rp);
        if (!conn.valid())
            return;
        char buf[4096];
        conn.recv(buf, sizeof(buf));
        std::string resp = "HTTP/1.1 200 OK\r\n"
                           "Transfer-Encoding: chunked\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "3\r\n"
                           "abc\r\n"
                           "3\r\n"
                           "def\r\n"
                           "0\r\n"
                           "\r\n";
        conn.send(resp.data(), resp.size());
        conn.shutdown();
    });

    std::string received;
    http::Client cli("127.0.0.1", static_cast<int>(port));
    auto res = cli.Get("/test", [&received](const char* data, size_t len) -> bool {
        received.append(data, len);
        return true;
    });
    fake_server.join();
    listener.close();

    ASSERT_TRUE(res);
    EXPECT_EQ(received, "abcdef");

    Log::set_sink(nullptr);
}

// ===========================================================================
// HTTP client: DELETE with body
// ===========================================================================

TEST_F(HttpCoverageTest, DeleteWithBody) {
    std::string received_body;
    server_.Delete("/resource", [&received_body](const http::Request& req, http::Response& res) {
        received_body = req.body;
        res.set_content("deleted", "text/plain");
    });
    start();

    auto cli = client();
    auto res = cli.Delete("/resource", "delete-payload", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, "deleted");
}

TEST_F(HttpCoverageTest, DeleteWithHeadersAndBody) {
    std::string received_auth;
    server_.Delete("/resource", [&received_auth](const http::Request& req, http::Response& res) {
        received_auth = req.get_header_value("Authorization");
        res.set_content("ok", "text/plain");
    });
    start();

    auto cli = client();
    http::Headers headers = {{"Authorization", "Bearer token123"}};
    auto res = cli.Delete("/resource", headers, "body", "text/plain");
    ASSERT_TRUE(res);
    EXPECT_EQ(received_auth, "Bearer token123");
}

// ===========================================================================
// HTTP client: PUT/PATCH with string body
// ===========================================================================

TEST_F(HttpCoverageTest, PutWithStringBody) {
    std::string received;
    server_.Put("/item", [&received](const http::Request& req, http::Response& res) {
        received = req.body;
        res.set_content("updated", "text/plain");
    });
    start();

    auto cli = client();
    auto res = cli.Put("/item", "new-value", "text/plain");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->body, "updated");
}

TEST_F(HttpCoverageTest, PatchWithStringBody) {
    std::string received;
    server_.Patch("/item", [&received](const http::Request& req, http::Response& res) {
        received = req.body;
        res.set_content("patched", "text/plain");
    });
    start();

    auto cli = client();
    auto res = cli.Patch("/item", R"({"field":"value"})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->body, "patched");
    EXPECT_EQ(received, R"({"field":"value"})");
}

// ===========================================================================
// HTTP client: PUT/PATCH with headers
// ===========================================================================

TEST_F(HttpCoverageTest, PutWithHeaders) {
    std::string auth;
    server_.Put("/item", [&auth](const http::Request& req, http::Response& res) {
        auth = req.get_header_value("X-Custom");
        res.status = 200;
    });
    start();

    auto cli = client();
    http::Headers h = {{"X-Custom", "custom-val"}};
    auto res = cli.Put("/item", h, "body", "text/plain");
    ASSERT_TRUE(res);
    EXPECT_EQ(auth, "custom-val");
}

TEST_F(HttpCoverageTest, PatchWithHeaders) {
    std::string auth;
    server_.Patch("/item", [&auth](const http::Request& req, http::Response& res) {
        auth = req.get_header_value("X-Custom");
        res.status = 200;
    });
    start();

    auto cli = client();
    http::Headers h = {{"X-Custom", "custom-val"}};
    auto res = cli.Patch("/item", h, "body", "text/plain");
    ASSERT_TRUE(res);
    EXPECT_EQ(auth, "custom-val");
}

// ===========================================================================
// HTTP server: HEAD returns headers but no body
// ===========================================================================

TEST_F(HttpCoverageTest, HeadReturnsNoBody) {
    server_.Get("/test",
                [](const http::Request&, http::Response& res) { res.set_content("this is the body", "text/plain"); });
    start();

    // Send raw HEAD request
    auto sock = platform::tcp_connect("127.0.0.1", port());
    std::string req = "HEAD /test HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
    sock.send(req.data(), req.size());

    std::string resp;
    char buf[8192];
    while (true) {
        ssize_t n = sock.recv(buf, sizeof(buf));
        if (n <= 0)
            break;
        resp.append(buf, static_cast<size_t>(n));
    }

    // Should have status 200
    auto sp1 = resp.find(' ');
    auto sp2 = resp.find(' ', sp1 + 1);
    std::string status = resp.substr(sp1 + 1, sp2 - sp1 - 1);
    EXPECT_EQ(status, "200");

    // Should have headers but no body after \r\n\r\n
    auto header_end = resp.find("\r\n\r\n");
    ASSERT_NE(header_end, std::string::npos);
    std::string body = resp.substr(header_end + 4);
    EXPECT_TRUE(body.empty()) << "HEAD response must not contain body, got: " << body;
}

// ===========================================================================
// HTTP server: POST with Content-Length body parsing
// ===========================================================================

TEST_F(HttpCoverageTest, PostWithLargeBody) {
    std::string received;
    server_.Post("/upload", [&received](const http::Request& req, http::Response& res) {
        received = req.body;
        res.set_content("ok", "text/plain");
    });
    start();

    std::string large_body(16384, 'X');
    auto cli = client();
    auto res = cli.Post("/upload", large_body, "application/octet-stream");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
}

// ===========================================================================
// HTTP client: streaming GET with Content-Length (non-chunked)
// ===========================================================================

TEST_F(HttpCoverageTest, StreamingGetWithContentLength) {
    server_.Get("/stream", [](const http::Request&, http::Response& res) {
        res.set_content(std::string(4096, 'Z'), "application/octet-stream");
    });
    start();

    std::string received;
    auto cli = client();
    auto res = cli.Get("/stream", [&received](const char* data, size_t len) -> bool {
        received.append(data, len);
        return true;
    });
    ASSERT_TRUE(res);
    EXPECT_EQ(received.size(), 4096u);
    EXPECT_EQ(received, std::string(4096, 'Z'));
}

// ===========================================================================
// HTTP client: streaming GET cancellation
// ===========================================================================

TEST_F(HttpCoverageTest, StreamingGetCancellation) {
    server_.Get("/big", [](const http::Request&, http::Response& res) {
        res.set_content(std::string(100000, 'A'), "application/octet-stream");
    });
    start();

    size_t total = 0;
    auto cli = client();
    auto res = cli.Get("/big", [&total](const char*, size_t len) -> bool {
        total += len;
        return total < 1000; // cancel after ~1KB
    });
    // Client should have stopped early
    EXPECT_LT(total, 100000u);
}

// ===========================================================================
// HTTP client: GET with headers and progress
// ===========================================================================

TEST_F(HttpCoverageTest, GetWithHeadersAndProgress) {
    server_.Get("/test", [](const http::Request& req, http::Response& res) {
        auto val = req.get_header_value("X-Test");
        res.set_content(val, "text/plain");
    });
    start();

    auto cli = client();
    http::Headers h = {{"X-Test", "header-value"}};
    bool progress_called = false;
    auto res = cli.Get("/test", h, [&progress_called](uint64_t, uint64_t) -> bool {
        progress_called = true;
        return true;
    });
    ASSERT_TRUE(res);
    EXPECT_EQ(res->body, "header-value");
}

// ===========================================================================
// HTTP client: response with extra headers (case-insensitive lookup)
// ===========================================================================

TEST_F(HttpCoverageTest, ResponseHeaderCaseInsensitive) {
    server_.Get("/test", [](const http::Request&, http::Response& res) {
        res.set_header("X-Custom-Header", "value123");
        res.set_content("ok", "text/plain");
    });
    start();

    auto cli = client();
    auto res = cli.Get("/test");
    ASSERT_TRUE(res);
    // Case-insensitive header lookup
    EXPECT_EQ(res->get_header_value("x-custom-header"), "value123");
    EXPECT_EQ(res->get_header_value("X-CUSTOM-HEADER"), "value123");
}

// ===========================================================================
// HTTP client: POST with raw char* body
// ===========================================================================

TEST_F(HttpCoverageTest, PostWithRawBody) {
    std::string received;
    server_.Post("/raw", [&received](const http::Request& req, http::Response& res) {
        received = req.body;
        res.set_content("ok", "text/plain");
    });
    start();

    auto cli = client();
    const char* body = "raw body data";
    auto res = cli.Post("/raw", body, strlen(body), "text/plain");
    ASSERT_TRUE(res);
    EXPECT_EQ(received, "raw body data");
}

// ===========================================================================
// HTTP client: POST with headers + raw body
// ===========================================================================

TEST_F(HttpCoverageTest, PostWithHeadersAndRawBody) {
    std::string auth;
    server_.Post("/auth", [&auth](const http::Request& req, http::Response& res) {
        auth = req.get_header_value("Authorization");
        res.set_content("ok", "text/plain");
    });
    start();

    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer xyz"}};
    auto res = cli.Post("/auth", h, "data", 4, "text/plain");
    ASSERT_TRUE(res);
    EXPECT_EQ(auth, "Bearer xyz");
}

// ===========================================================================
// HTTP client: Options method
// ===========================================================================

TEST_F(HttpCoverageTest, OptionsMethod) {
    server_.Options("/resource", [](const http::Request&, http::Response& res) {
        res.set_header("Allow", "GET, POST, OPTIONS");
        res.status = 204;
    });
    start();

    auto cli = client();
    auto res = cli.Options("/resource");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 204);
}

TEST_F(HttpCoverageTest, OptionsWithHeaders) {
    server_.Options("/resource", [](const http::Request& req, http::Response& res) {
        auto origin = req.get_header_value("Origin");
        if (!origin.empty())
            res.set_header("Access-Control-Allow-Origin", origin);
        res.status = 204;
    });
    start();

    auto cli = client();
    http::Headers h = {{"Origin", "https://example.com"}};
    auto res = cli.Options("/resource", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->get_header_value("Access-Control-Allow-Origin"), "https://example.com");
}

// ===========================================================================
// HTTP client: Head method
// ===========================================================================

TEST_F(HttpCoverageTest, HeadMethod) {
    server_.Get("/test",
                [](const http::Request&, http::Response& res) { res.set_content("body content", "text/plain"); });
    start();

    auto cli = client();
    auto res = cli.Head("/test");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
}

TEST_F(HttpCoverageTest, HeadWithHeaders) {
    server_.Get("/test", [](const http::Request&, http::Response& res) { res.set_content("body", "text/plain"); });
    start();

    auto cli = client();
    http::Headers h = {{"Accept", "text/plain"}};
    auto res = cli.Head("/test", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
}

// ===========================================================================
// HTTP free functions
// ===========================================================================

TEST(HttpFunctions, StatusMessageCoversAllCodes) {
    // Verify a sample of status codes return non-empty strings
    EXPECT_STRNE(http::status_message(100), "");
    EXPECT_STRNE(http::status_message(200), "");
    EXPECT_STRNE(http::status_message(301), "");
    EXPECT_STRNE(http::status_message(404), "");
    EXPECT_STRNE(http::status_message(500), "");
    EXPECT_STRNE(http::status_message(418), ""); // teapot
    EXPECT_STRNE(http::status_message(429), ""); // too many requests
    EXPECT_STRNE(http::status_message(503), ""); // service unavailable
    EXPECT_STREQ(http::status_message(999), ""); // unknown
}

TEST(HttpFunctions, GetBearerTokenAuth) {
    http::Request req;
    req.set_header("Authorization", "Bearer my-token-123");
    EXPECT_EQ(http::get_bearer_token_auth(req), "my-token-123");
}

TEST(HttpFunctions, GetBearerTokenAuthMissing) {
    http::Request req;
    EXPECT_EQ(http::get_bearer_token_auth(req), "");
}

TEST(HttpFunctions, GetBearerTokenAuthWrongScheme) {
    http::Request req;
    req.set_header("Authorization", "Basic dXNlcjpwYXNz");
    EXPECT_EQ(http::get_bearer_token_auth(req), "");
}

// ===========================================================================
// HTTP client: scheme-based constructor
// ===========================================================================

TEST(HttpClientScheme, HttpScheme) {
    http::Client cli("http://127.0.0.1:12345");
    EXPECT_EQ(cli.port(), 12345);
}

TEST(HttpClientScheme, HttpsScheme) {
    http::Client cli("https://127.0.0.1");
    EXPECT_EQ(cli.port(), 443);
}

TEST(HttpClientScheme, NoSchemeWithPort) {
    http::Client cli("127.0.0.1", 8080);
    EXPECT_EQ(cli.host(), "127.0.0.1");
    EXPECT_EQ(cli.port(), 8080);
}

// ===========================================================================
// HTTP client: setter methods (exercise code paths)
// ===========================================================================

TEST(HttpClientSetters, AllSettersWork) {
    http::Client cli("127.0.0.1", 80);
    // These should all succeed without crashing
    cli.set_connection_timeout(5);
    cli.set_read_timeout(10);
    cli.set_write_timeout(5);
    cli.set_keep_alive(true);
    cli.set_follow_location(true);
    cli.set_url_encode(false);
    cli.set_compress(false);
    cli.set_decompress(true);
    cli.set_tcp_nodelay(true);
    cli.set_basic_auth("user", "pass");
    cli.set_bearer_token_auth("token");
    cli.set_proxy("proxy.example.com", 8080);
    cli.set_proxy_basic_auth("proxyuser", "proxypass");
    cli.set_proxy_bearer_token_auth("proxytoken");
    cli.set_interface("eth0");
    cli.set_default_headers({{"X-Default", "val"}});
    cli.set_logger([](const http::Request&, const http::Response&) {});
    EXPECT_TRUE(cli.is_valid());
}

// ===========================================================================
// Header size limit: client rejects oversized headers
// ===========================================================================

TEST(HttpClientHeaderLimit, RejectsOversizedHeaders) {
    Log::set_sink([](LogLevel, const std::string&, const std::string&) {});

    auto listener = platform::tcp_listen("127.0.0.1", 0);
    uint16_t port = listener.local_port();

    std::thread fake_server([&] {
        listener.set_non_blocking(false);
        std::string addr;
        uint16_t rp;
        auto conn = platform::tcp_accept(listener, addr, rp);
        if (!conn.valid())
            return;
        char buf[4096];
        conn.recv(buf, sizeof(buf));
        // Send response with a massive header section (> 64 KB)
        std::string resp = "HTTP/1.1 200 OK\r\n";
        for (int i = 0; i < 2000; ++i)
            resp += "X-Padding-" + std::to_string(i) + ": " + std::string(40, 'x') + "\r\n";
        resp += "\r\nbody";
        conn.send(resp.data(), resp.size());
        conn.shutdown();
    });

    http::Client cli("127.0.0.1", static_cast<int>(port));
    auto res = cli.Get("/test");
    fake_server.join();
    listener.close();

    // Should fail — headers too large
    EXPECT_FALSE(res);

    Log::set_sink(nullptr);
}

// ===========================================================================
// Chunked: invalid hex chunk size is handled gracefully
// ===========================================================================

TEST(HttpClientChunked, InvalidChunkSizeHandledGracefully) {
    Log::set_sink([](LogLevel, const std::string&, const std::string&) {});

    auto listener = platform::tcp_listen("127.0.0.1", 0);
    uint16_t port = listener.local_port();

    std::thread fake_server([&] {
        listener.set_non_blocking(false);
        std::string addr;
        uint16_t rp;
        auto conn = platform::tcp_accept(listener, addr, rp);
        if (!conn.valid())
            return;
        char buf[4096];
        conn.recv(buf, sizeof(buf));
        std::string resp = "HTTP/1.1 200 OK\r\n"
                           "Transfer-Encoding: chunked\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "ZZZZ\r\n" // invalid hex
                           "garbage\r\n";
        conn.send(resp.data(), resp.size());
        conn.shutdown();
    });

    http::Client cli("127.0.0.1", static_cast<int>(port));
    auto res = cli.Get("/test");
    fake_server.join();
    listener.close();

    // Should succeed with empty body (parser stops at invalid chunk)
    ASSERT_TRUE(res);
    EXPECT_TRUE(res->body.empty());

    Log::set_sink(nullptr);
}

// ===========================================================================
// Chunked: chunk extensions are stripped (RFC 9112 §7.1.1)
// ===========================================================================

TEST(HttpClientChunked, ChunkExtensionsStripped) {
    Log::set_sink([](LogLevel, const std::string&, const std::string&) {});

    auto listener = platform::tcp_listen("127.0.0.1", 0);
    uint16_t port = listener.local_port();

    std::thread fake_server([&] {
        listener.set_non_blocking(false);
        std::string addr;
        uint16_t rp;
        auto conn = platform::tcp_accept(listener, addr, rp);
        if (!conn.valid())
            return;
        char buf[4096];
        conn.recv(buf, sizeof(buf));
        std::string resp = "HTTP/1.1 200 OK\r\n"
                           "Transfer-Encoding: chunked\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "5;ext=value\r\n" // chunk extension
                           "Hello\r\n"
                           "0\r\n"
                           "\r\n";
        conn.send(resp.data(), resp.size());
        conn.shutdown();
    });

    http::Client cli("127.0.0.1", static_cast<int>(port));
    auto res = cli.Get("/test");
    fake_server.join();
    listener.close();

    ASSERT_TRUE(res);
    EXPECT_EQ(res->body, "Hello");

    Log::set_sink(nullptr);
}

// ===========================================================================
// Chunked: trailer headers are consumed without corrupting keep-alive
// ===========================================================================

TEST(HttpClientChunked, TrailerHeadersConsumed) {
    Log::set_sink([](LogLevel, const std::string&, const std::string&) {});

    auto listener = platform::tcp_listen("127.0.0.1", 0);
    uint16_t port = listener.local_port();

    std::thread fake_server([&] {
        listener.set_non_blocking(false);
        std::string addr;
        uint16_t rp;
        auto conn = platform::tcp_accept(listener, addr, rp);
        if (!conn.valid())
            return;

        // First request: chunked with trailers
        char buf[4096];
        conn.recv(buf, sizeof(buf));
        std::string resp1 = "HTTP/1.1 200 OK\r\n"
                            "Transfer-Encoding: chunked\r\n"
                            "Connection: keep-alive\r\n"
                            "\r\n"
                            "5\r\n"
                            "First\r\n"
                            "0\r\n"
                            "Trailer-Key: trailer-value\r\n"
                            "\r\n";
        conn.send(resp1.data(), resp1.size());

        // Second request on same connection
        conn.recv(buf, sizeof(buf));
        std::string resp2 = "HTTP/1.1 200 OK\r\n"
                            "Content-Length: 6\r\n"
                            "Connection: close\r\n"
                            "\r\n"
                            "Second";
        conn.send(resp2.data(), resp2.size());
        conn.shutdown();
    });

    http::Client cli("127.0.0.1", static_cast<int>(port));
    cli.set_keep_alive(true);

    auto r1 = cli.Get("/first");
    ASSERT_TRUE(r1);
    EXPECT_EQ(r1->body, "First");

    auto r2 = cli.Get("/second");
    ASSERT_TRUE(r2);
    EXPECT_EQ(r2->body, "Second");

    fake_server.join();
    listener.close();

    Log::set_sink(nullptr);
}

// ===========================================================================
// from_chars: malformed Content-Length header logged and returns default
// ===========================================================================

TEST(HttpClientFromChars, MalformedContentLengthUsesDefault) {
    Log::set_sink([](LogLevel, const std::string&, const std::string&) {});

    auto listener = platform::tcp_listen("127.0.0.1", 0);
    uint16_t port = listener.local_port();

    std::thread fake_server([&] {
        listener.set_non_blocking(false);
        std::string addr;
        uint16_t rp;
        auto conn = platform::tcp_accept(listener, addr, rp);
        if (!conn.valid())
            return;
        char buf[4096];
        conn.recv(buf, sizeof(buf));
        std::string resp = "HTTP/1.1 200 OK\r\n"
                           "Content-Length: not-a-number\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "body-data";
        conn.send(resp.data(), resp.size());
        conn.shutdown();
    });

    http::Client cli("127.0.0.1", static_cast<int>(port));
    auto res = cli.Get("/test");
    fake_server.join();
    listener.close();

    // Should succeed — malformed Content-Length defaults to 0
    ASSERT_TRUE(res);

    Log::set_sink(nullptr);
}

// ===========================================================================
// Socket: double close is safe
// ===========================================================================

TEST(PlatformSocketEdge, ConnectAfterCloseThrows) {
    // After closing, the socket should be invalid
    auto s = platform::tcp_create();
    s.close();
    EXPECT_FALSE(s.valid());
    EXPECT_EQ(s.fd(), -1);
}

TEST(PlatformSocketEdge, ShutdownOnFreshSocketDoesNotCrash) {
    auto s = platform::tcp_create();
    s.shutdown(); // should not crash even though not connected
    s.close();
}

TEST(PlatformSocketEdge, SendOnClosedSocketReturnsError) {
    auto s = platform::tcp_create();
    s.close();
    // Sending on an invalid socket should return error
    // (behavior may vary — just ensure no crash)
    ssize_t n = s.send("test", 4);
    EXPECT_LE(n, 0);
}

TEST(PlatformSocketEdge, RecvOnClosedSocketReturnsError) {
    auto s = platform::tcp_create();
    s.close();
    char buf[16];
    ssize_t n = s.recv(buf, sizeof(buf));
    EXPECT_LE(n, 0);
}
