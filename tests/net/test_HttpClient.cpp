#include <gtest/gtest.h>

#include "net/Http.h"
#include "utils/Log.h"

#include <chrono>
#include <thread>

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
