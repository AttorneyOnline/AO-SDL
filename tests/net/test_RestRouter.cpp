#include <gtest/gtest.h>

#include "game/GameRoom.h"
#include "game/ServerSession.h"
#include "net/EndpointFactory.h"
#include "net/RestEndpoint.h"
#include "net/RestRouter.h"
#include "net/nx/NXEndpoint.h"
#include "net/nx/NXServer.h"
#include "utils/Log.h"

#include "net/Http.h"

#include <set>
#include <thread>

// -- Test helpers ------------------------------------------------------------

/// Minimal endpoint for testing.
class StubEndpoint : public RestEndpoint {
  public:
    StubEndpoint(std::string m, std::string p, bool auth, RestResponse resp)
        : method_(std::move(m)), path_(std::move(p)), auth_(auth), response_(std::move(resp)) {
    }

    const std::string& method() const override {
        return method_;
    }
    const std::string& path_pattern() const override {
        return path_;
    }
    bool requires_auth() const override {
        return auth_;
    }
    RestResponse handle(const RestRequest& req) override {
        last_path = req.path;
        last_path_params = req.path_params;
        last_query_params = req.query_params;
        last_body = req.body;
        last_session = req.session;
        return response_;
    }

    std::string last_path;
    std::unordered_map<std::string, std::string> last_path_params;
    std::unordered_map<std::string, std::string> last_query_params;
    std::optional<nlohmann::json> last_body;
    ServerSession* last_session = nullptr;

  private:
    std::string method_;
    std::string path_;
    bool auth_;
    RestResponse response_;
};

// -- Test fixture ------------------------------------------------------------

class RestRouterTest : public ::testing::Test {
  protected:
    void SetUp() override {
        Log::set_sink([](LogLevel, const std::string&, const std::string&) {});
    }

    void TearDown() override {
        if (server_thread_.joinable()) {
            http_.stop();
            server_thread_.join();
        }
        Log::set_sink(nullptr);
    }

    void start() {
        router_.bind(http_);
        port_ = http_.bind_to_any_port("127.0.0.1");
        ASSERT_GT(port_, 0);
        server_thread_ = std::thread([this] { http_.listen_after_bind(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    http::Client client() {
        return http::Client("127.0.0.1", port_);
    }

    http::Server http_;
    RestRouter router_;
    int port_ = 0;
    std::thread server_thread_;

    // Shared room and session for auth tests
    GameRoom room_;
};

// -- Tests -------------------------------------------------------------------

TEST_F(RestRouterTest, SimpleGetEndpoint) {
    auto ep = std::make_unique<StubEndpoint>("GET", "/test", false, RestResponse::json(200, {{"ok", true}}));
    auto* ep_ptr = ep.get();
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    auto res = cli.Get("/test");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    auto body = nlohmann::json::parse(res->body);
    EXPECT_EQ(body["ok"], true);
    EXPECT_EQ(ep_ptr->last_path, "/test");
}

TEST_F(RestRouterTest, PostWithJsonBody) {
    auto ep = std::make_unique<StubEndpoint>("POST", "/data", false, RestResponse::json(201, {{"created", true}}));
    auto* ep_ptr = ep.get();
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    auto res = cli.Post("/data", R"({"name":"test"})", "application/json");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 201);
    ASSERT_TRUE(ep_ptr->last_body.has_value());
    EXPECT_EQ((*ep_ptr->last_body)["name"], "test");
}

TEST_F(RestRouterTest, MalformedJsonReturns400) {
    auto ep = std::make_unique<StubEndpoint>("POST", "/data", false, RestResponse::json(200, {}));
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    auto res = cli.Post("/data", "not json{", "application/json");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    auto body = nlohmann::json::parse(res->body);
    EXPECT_NE(body["reason"].get<std::string>().find("Malformed"), std::string::npos);
}

TEST_F(RestRouterTest, PathParameters) {
    auto ep = std::make_unique<StubEndpoint>("GET", "/items/:id", false, RestResponse::json(200, {}));
    auto* ep_ptr = ep.get();
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    auto res = cli.Get("/items/42");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(ep_ptr->last_path_params["id"], "42");
}

TEST_F(RestRouterTest, QueryParameters) {
    auto ep = std::make_unique<StubEndpoint>("GET", "/search", false, RestResponse::json(200, {}));
    auto* ep_ptr = ep.get();
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    auto res = cli.Get("/search?q=hello&limit=10");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(ep_ptr->last_query_params["q"], "hello");
    EXPECT_EQ(ep_ptr->last_query_params["limit"], "10");
}

TEST_F(RestRouterTest, NoContent204) {
    auto ep = std::make_unique<StubEndpoint>("DELETE", "/thing", false, RestResponse::no_content());
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    auto res = cli.Delete("/thing");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 204);
}

TEST_F(RestRouterTest, AuthRequiredWithoutToken) {
    auto ep = std::make_unique<StubEndpoint>("GET", "/secret", true, RestResponse::json(200, {}));
    router_.register_endpoint(std::move(ep));
    router_.set_auth_func([](const std::string&) -> ServerSession* { return nullptr; });

    start();
    auto cli = client();
    auto res = cli.Get("/secret");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
    auto body = nlohmann::json::parse(res->body);
    EXPECT_NE(body["reason"].get<std::string>().find("Missing"), std::string::npos);
}

TEST_F(RestRouterTest, AuthRequiredWithBadToken) {
    auto ep = std::make_unique<StubEndpoint>("GET", "/secret", true, RestResponse::json(200, {}));
    router_.register_endpoint(std::move(ep));
    router_.set_auth_func([](const std::string&) -> ServerSession* { return nullptr; });

    start();
    auto cli = client();
    http::Headers headers = {{"Authorization", "Bearer badtoken"}};
    auto res = cli.Get("/secret", headers);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
    auto body = nlohmann::json::parse(res->body);
    EXPECT_NE(body["reason"].get<std::string>().find("Invalid"), std::string::npos);
}

TEST_F(RestRouterTest, AuthSuccessPassesSession) {
    auto ep = std::make_unique<StubEndpoint>("GET", "/secret", true, RestResponse::json(200, {{"ok", true}}));
    auto* ep_ptr = ep.get();
    router_.register_endpoint(std::move(ep));

    auto session = room_.create_session(1, "aonx");
    session->session_token = "validtoken";
    room_.register_session_token("validtoken", 1);
    session->joined = true;

    router_.set_auth_func(
        [this](const std::string& token) -> ServerSession* { return room_.find_session_by_token(token); });

    start();
    auto cli = client();
    http::Headers headers = {{"Authorization", "Bearer validtoken"}};
    auto res = cli.Get("/secret", headers);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    ASSERT_NE(ep_ptr->last_session, nullptr);
    EXPECT_EQ(ep_ptr->last_session->session_token, "validtoken");
}

TEST_F(RestRouterTest, AuthTouchesSession) {
    auto ep = std::make_unique<StubEndpoint>("GET", "/secret", true, RestResponse::json(200, {}));
    router_.register_endpoint(std::move(ep));

    auto session = room_.create_session(1, "aonx");
    session->session_token = "tok";
    room_.register_session_token("tok", 1);
    session->joined = true;

    // Backdate the last_activity
    session->last_activity_ns.store(
        (std::chrono::steady_clock::now() - std::chrono::seconds(100)).time_since_epoch().count(),
        std::memory_order_relaxed);
    auto old_activity = session->last_activity();

    router_.set_auth_func(
        [this](const std::string& token) -> ServerSession* { return room_.find_session_by_token(token); });

    start();
    auto cli = client();
    http::Headers headers = {{"Authorization", "Bearer tok"}};
    auto res = cli.Get("/secret", headers);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_GT(session->last_activity(), old_activity);
}

TEST_F(RestRouterTest, DeleteMethod) {
    auto ep = std::make_unique<StubEndpoint>("DELETE", "/items/:id", false, RestResponse::no_content());
    auto* ep_ptr = ep.get();
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    auto res = cli.Delete("/items/99");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 204);
    EXPECT_EQ(ep_ptr->last_path_params["id"], "99");
}

TEST_F(RestRouterTest, PatchMethod) {
    auto ep = std::make_unique<StubEndpoint>("PATCH", "/items/:id", false, RestResponse::json(200, {}));
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    auto res = cli.Patch("/items/5", R"({"name":"updated"})", "application/json");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
}

TEST_F(RestRouterTest, PutMethod) {
    auto ep = std::make_unique<StubEndpoint>("PUT", "/items/:id", false, RestResponse::json(200, {}));
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    auto res = cli.Put("/items/5", R"({"name":"replaced"})", "application/json");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
}

// -- CORS tests --------------------------------------------------------------

/// Helper: count occurrences of a header in a response.
static size_t header_count(const http::Result& res, const std::string& key) {
    return res->get_header_value_count(key);
}

TEST_F(RestRouterTest, CorsWildcardOnGet) {
    router_.set_cors_origins({"*"});
    auto ep = std::make_unique<StubEndpoint>("GET", "/test", false, RestResponse::json(200, {{"ok", true}}));
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    auto res = cli.Get("/test");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->get_header_value("Access-Control-Allow-Origin"), "*");
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Origin"), 1u);
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Methods"), 1u);
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Headers"), 1u);
}

TEST_F(RestRouterTest, CorsWildcardOnPreflight) {
    router_.set_cors_origins({"*"});
    auto ep = std::make_unique<StubEndpoint>("POST", "/data", false, RestResponse::json(200, {}));
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    auto res = cli.Options("/data");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 204);
    EXPECT_EQ(res->get_header_value("Access-Control-Allow-Origin"), "*");
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Origin"), 1u);
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Methods"), 1u);
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Headers"), 1u);
}

TEST_F(RestRouterTest, CorsWildcardInArrayTreatedAsWildcard) {
    // If "*" appears anywhere in the list, treat the whole config as wildcard.
    router_.set_cors_origins({"https://a.com", "*", "https://b.com"});
    auto ep = std::make_unique<StubEndpoint>("GET", "/test", false, RestResponse::json(200, {}));
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    auto res = cli.Get("/test");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->get_header_value("Access-Control-Allow-Origin"), "*");
}

TEST_F(RestRouterTest, CorsSingleOrigin) {
    router_.set_cors_origins({"https://example.com"});
    auto ep = std::make_unique<StubEndpoint>("GET", "/test", false, RestResponse::json(200, {}));
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    auto res = cli.Get("/test");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->get_header_value("Access-Control-Allow-Origin"), "https://example.com");
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Origin"), 1u);
    // Single origin should NOT have Vary: Origin
    EXPECT_EQ(header_count(res, "Vary"), 0u);
}

TEST_F(RestRouterTest, CorsMultipleOriginsMatchesRequest) {
    router_.set_cors_origins({"https://a.com", "https://b.com", "https://c.com"});
    auto ep = std::make_unique<StubEndpoint>("GET", "/test", false, RestResponse::json(200, {}));
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    http::Headers headers = {{"Origin", "https://b.com"}};
    auto res = cli.Get("/test", headers);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->get_header_value("Access-Control-Allow-Origin"), "https://b.com");
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Origin"), 1u);
    EXPECT_EQ(res->get_header_value("Vary"), "Origin");
}

TEST_F(RestRouterTest, CorsMultipleOriginsRejectsUnlisted) {
    router_.set_cors_origins({"https://a.com", "https://b.com"});
    auto ep = std::make_unique<StubEndpoint>("GET", "/test", false, RestResponse::json(200, {}));
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    http::Headers headers = {{"Origin", "https://evil.com"}};
    auto res = cli.Get("/test", headers);

    ASSERT_TRUE(res);
    // No Allow-Origin header for unmatched origins
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Origin"), 0u);
    // Vary: Origin is still present (caching correctness)
    EXPECT_EQ(res->get_header_value("Vary"), "Origin");
}

TEST_F(RestRouterTest, CorsMultipleOriginsOnPreflight) {
    router_.set_cors_origins({"https://a.com", "https://b.com"});
    auto ep = std::make_unique<StubEndpoint>("POST", "/data", false, RestResponse::json(200, {}));
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    http::Headers headers = {{"Origin", "https://a.com"}};
    auto res = cli.Options("/data", headers);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 204);
    EXPECT_EQ(res->get_header_value("Access-Control-Allow-Origin"), "https://a.com");
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Origin"), 1u);
    EXPECT_EQ(res->get_header_value("Vary"), "Origin");
}

TEST_F(RestRouterTest, CorsDisabledSendsNoHeaders) {
    // Empty origins = CORS disabled
    router_.set_cors_origins({});
    auto ep = std::make_unique<StubEndpoint>("GET", "/test", false, RestResponse::json(200, {}));
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    auto res = cli.Get("/test");

    ASSERT_TRUE(res);
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Origin"), 0u);
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Methods"), 0u);
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Headers"), 0u);
}

TEST_F(RestRouterTest, CorsNoDuplicateHeadersOnPreflight) {
    // Regression test: previously set_default_headers + set_cors() caused duplicates.
    router_.set_cors_origins({"https://example.com"});
    auto ep = std::make_unique<StubEndpoint>("POST", "/data", false, RestResponse::json(200, {}));
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    auto res = cli.Options("/data");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 204);
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Origin"), 1u);
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Methods"), 1u);
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Headers"), 1u);
}

TEST_F(RestRouterTest, CorsNoDuplicateHeadersOnRegularRequest) {
    router_.set_cors_origins({"https://example.com"});
    auto ep = std::make_unique<StubEndpoint>("GET", "/test", false, RestResponse::json(200, {}));
    router_.register_endpoint(std::move(ep));

    start();
    auto cli = client();
    auto res = cli.Get("/test");

    ASSERT_TRUE(res);
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Origin"), 1u);
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Methods"), 1u);
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Headers"), 1u);
}

TEST_F(RestRouterTest, CorsHeadersOnUnmatchedRoute) {
    // CORS headers should appear even on 404 responses.
    router_.set_cors_origins({"*"});
    // No endpoints registered — any path is unmatched.

    start();
    auto cli = client();
    auto res = cli.Get("/nonexistent");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
    EXPECT_EQ(res->get_header_value("Access-Control-Allow-Origin"), "*");
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Origin"), 1u);
}

TEST_F(RestRouterTest, CorsPreflightOnUnmatchedRouteStillHasHeaders) {
    // OPTIONS to a path with no registered endpoint returns 404 (no catch-all
    // regex support in this httplib), but default_headers still inject CORS.
    router_.set_cors_origins({"https://example.com"});

    start();
    auto cli = client();
    auto res = cli.Options("/nonexistent");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
    EXPECT_EQ(res->get_header_value("Access-Control-Allow-Origin"), "https://example.com");
    EXPECT_EQ(header_count(res, "Access-Control-Allow-Origin"), 1u);
}

// -- GameRoom session tests --------------------------------------------------

TEST(GameRoomTest, FindSessionByToken) {
    GameRoom room;
    room.areas = {"Lobby"};
    auto s1 = room.create_session(1, "aonx");
    s1->session_token = "token_a";
    room.register_session_token("token_a", 1);
    auto s2 = room.create_session(2, "aonx");
    s2->session_token = "token_b";
    room.register_session_token("token_b", 2);

    EXPECT_EQ(room.find_session_by_token("token_a"), s1.get());
    EXPECT_EQ(room.find_session_by_token("token_b"), s2.get());
    EXPECT_EQ(room.find_session_by_token("nonexistent"), nullptr);
}

TEST(GameRoomTest, ExpireSessionsRemovesStale) {
    GameRoom room;
    room.areas = {"Lobby"};
    room.characters = {"Phoenix"};
    room.reset_taken();

    auto s1 = room.create_session(1, "aonx");
    s1->session_token = "active";
    room.register_session_token("active", 1);
    s1->touch();

    auto s2 = room.create_session(2, "aonx");
    s2->session_token = "stale";
    room.register_session_token("stale", 2);
    s2->character_id = 0;
    room.char_taken[0] = 1;
    s2->last_activity_ns.store(
        (std::chrono::steady_clock::now() - std::chrono::seconds(600)).time_since_epoch().count(),
        std::memory_order_relaxed);

    EXPECT_EQ(room.session_count(), 2u);
    Log::set_sink([](LogLevel, const std::string&, const std::string&) {});
    int expired = room.expire_sessions(300);
    Log::set_sink(nullptr);

    EXPECT_EQ(expired, 1);
    EXPECT_EQ(room.session_count(), 1u);
    EXPECT_NE(room.find_session_by_token("active"), nullptr);
    EXPECT_EQ(room.find_session_by_token("stale"), nullptr);
    EXPECT_EQ(room.char_taken[0], 0); // Character freed
}

TEST(GameRoomTest, ExpireSessionsSkipsAO2) {
    GameRoom room;
    room.areas = {"Lobby"};

    // AO2 session has empty token — should never be expired
    auto s = room.create_session(1, "ao2");
    s->last_activity_ns.store(
        (std::chrono::steady_clock::now() - std::chrono::seconds(9999)).time_since_epoch().count(),
        std::memory_order_relaxed);

    Log::set_sink([](LogLevel, const std::string&, const std::string&) {});
    int expired = room.expire_sessions(300);
    Log::set_sink(nullptr);

    EXPECT_EQ(expired, 0);
    EXPECT_EQ(room.session_count(), 1u);
}

// -- RestResponse tests ------------------------------------------------------

TEST(RestResponseTest, JsonFactory) {
    auto resp = RestResponse::json(201, {{"id", 42}});
    EXPECT_EQ(resp.status, 201);
    EXPECT_EQ(resp.body["id"], 42);
    EXPECT_EQ(resp.content_type, "application/json");
}

TEST(RestResponseTest, ErrorFactory) {
    auto resp = RestResponse::error(404, "Not found");
    EXPECT_EQ(resp.status, 404);
    EXPECT_EQ(resp.body["reason"], "Not found");
}

TEST(RestResponseTest, NoContentFactory) {
    auto resp = RestResponse::no_content();
    EXPECT_EQ(resp.status, 204);
    EXPECT_TRUE(resp.body.is_null());
}

// -- EndpointFactory / EndpointRegistrar tests -------------------------------

TEST(EndpointFactoryTest, RouterServesRegisteredEndpoint) {
    RestRouter router;
    auto ep =
        std::make_unique<StubEndpoint>("GET", "/factory-test", false, RestResponse::json(200, {{"from", "factory"}}));
    router.register_endpoint(std::move(ep));

    // Verify the router received it by binding and hitting the endpoint.
    http::Server http;
    router.bind(http);
    int port = http.bind_to_any_port("127.0.0.1");
    ASSERT_GT(port, 0);

    std::thread t([&] { http.listen_after_bind(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    http::Client cli("127.0.0.1", port);
    auto res = cli.Get("/factory-test");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    auto body = nlohmann::json::parse(res->body);
    EXPECT_EQ(body["from"], "factory");

    http.stop();
    t.join();
}

TEST(EndpointFactoryTest, GlobalRegistrarPopulatesToRouter) {
    // The global EndpointFactory already has registrations from the NX endpoint
    // TUs (ServerPlayersEndpoint, SessionCreateEndpoint, SessionDeleteEndpoint).
    // Force-link the endpoint TUs (same as main.cpp does).
    nx_register_endpoints();

    Log::set_sink([](LogLevel, const std::string&, const std::string&) {});

    GameRoom room;
    room.areas = {"Lobby"};
    room.max_players = 50;
    NXServer nx(room);
    NXEndpoint::set_server(&nx);

    RestRouter router;
    EndpointFactory::instance().populate(router);

    http::Server http;
    router.bind(http);
    int port = http.bind_to_any_port("127.0.0.1");
    ASSERT_GT(port, 0);

    std::thread t([&] { http.listen_after_bind(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    http::Client cli("127.0.0.1", port);

    // The global factory should have registered GET /aonx/v1/server
    auto res = cli.Get("/aonx/v1/server");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    auto body = nlohmann::json::parse(res->body);
    EXPECT_EQ(body["online"], 0);
    EXPECT_EQ(body["max"], 50);

    http.stop();
    t.join();
    Log::set_sink(nullptr);
}

TEST(EndpointFactoryTest, SessionCreateRejects503WhenFull) {
    nx_register_endpoints();
    Log::set_sink([](LogLevel, const std::string&, const std::string&) {});

    GameRoom room;
    room.areas = {"Lobby"};
    room.max_players = 1;
    NXServer nx(room);
    NXEndpoint::set_server(&nx);

    RestRouter router;
    EndpointFactory::instance().populate(router);

    http::Server http;
    router.bind(http);
    int port = http.bind_to_any_port("127.0.0.1");
    ASSERT_GT(port, 0);

    std::thread t([&] { http.listen_after_bind(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    http::Client cli("127.0.0.1", port);
    std::string session_body = R"({"client_name":"test","client_version":"1.0","hdid":"abc"})";

    // First session should succeed
    auto res1 = cli.Post("/aonx/v1/session", session_body, "application/json");
    ASSERT_TRUE(res1);
    EXPECT_EQ(res1->status, 201);

    // Second session should be rejected — server is full (max_players=1)
    auto res2 = cli.Post("/aonx/v1/session", session_body, "application/json");
    ASSERT_TRUE(res2);
    EXPECT_EQ(res2->status, 503);
    auto body = nlohmann::json::parse(res2->body);
    EXPECT_EQ(body["reason"], "Server is full");

    http.stop();
    t.join();
    Log::set_sink(nullptr);
}

// -- with_lock concurrency test ----------------------------------------------

TEST_F(RestRouterTest, WithLockSerializesAccess) {
    // Register an endpoint that increments a shared counter.
    // Fire concurrent requests and verify the final count is correct
    // (no lost increments), proving the mutex serializes access.
    std::atomic<int> counter{0};

    struct CounterEndpoint : public RestEndpoint {
        std::atomic<int>& counter_ref;
        explicit CounterEndpoint(std::atomic<int>& c) : counter_ref(c) {
        }
        const std::string& method() const override {
            static const std::string m = "POST";
            return m;
        }
        const std::string& path_pattern() const override {
            static const std::string p = "/inc";
            return p;
        }
        bool requires_auth() const override {
            return false;
        }
        RestResponse handle(const RestRequest&) override {
            // Non-atomic read-modify-write to detect unserialized access.
            // If the mutex is working, this is safe because only one
            // handler runs at a time.
            int val = counter_ref.load(std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            counter_ref.store(val + 1, std::memory_order_relaxed);
            return RestResponse::json(200, {{"count", val + 1}});
        }
    };

    router_.register_endpoint(std::make_unique<CounterEndpoint>(counter));
    start();

    constexpr int N = 20;
    std::vector<std::thread> threads;
    threads.reserve(N);
    for (int i = 0; i < N; ++i) {
        threads.emplace_back([this] {
            auto cli = client();
            cli.Post("/inc", "", "application/json");
        });
    }
    for (auto& t : threads)
        t.join();

    EXPECT_EQ(counter.load(), N);
}

// -- with_lock direct API test -----------------------------------------------

TEST_F(RestRouterTest, WithLockExecutesUnderMutex) {
    // Verify with_lock() actually runs the callable.
    bool executed = false;
    router_.with_lock([&] { executed = true; });
    EXPECT_TRUE(executed);
}

// -- NX endpoint test fixture -------------------------------------------------
// Eliminates per-test boilerplate: sets up GameRoom, NXServer, RestRouter,
// httplib server, and provides a client() helper and create_session().

class NXEndpointTest : public ::testing::Test {
  protected:
    void SetUp() override {
        nx_register_endpoints();
        Log::set_sink([](LogLevel, const std::string&, const std::string&) {});
        room_.characters = {"Phoenix", "Edgeworth", "Maya"};
        room_.areas = {"Lobby", "Courtroom 1"};
        room_.reset_taken();
        room_.build_char_id_index();
        room_.build_area_index();
        nx_ = std::make_unique<NXServer>(room_);
        NXEndpoint::set_server(nx_.get());
        router_.set_auth_func(
            [this](const std::string& token) -> ServerSession* { return room_.find_session_by_token(token); });
        EndpointFactory::instance().populate(router_);
        router_.bind(http_);
        port_ = http_.bind_to_any_port("127.0.0.1");
        server_thread_ = std::thread([this] { http_.listen_after_bind(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    void TearDown() override {
        http_.stop();
        if (server_thread_.joinable())
            server_thread_.join();
        Log::set_sink(nullptr);
    }

    http::Client client() {
        return http::Client("127.0.0.1", port_);
    }

    /// Create a session and return the bearer token.
    std::string create_session() {
        auto cli = client();
        auto res = cli.Post("/aonx/v1/session", R"({"client_name":"test","client_version":"1.0","hdid":"abc"})",
                            "application/json");
        return nlohmann::json::parse(res->body)["token"].get<std::string>();
    }

    GameRoom room_;
    std::unique_ptr<NXServer> nx_;
    RestRouter router_;
    http::Server http_;
    int port_ = 0;
    std::thread server_thread_;
};

// -- Phase 2 endpoint tests --------------------------------------------------

TEST_F(NXEndpointTest, SessionRenewExtendsTTL) {
    nx_->set_session_ttl_seconds(600);
    auto token = create_session();

    auto cli = client();
    http::Headers headers = {{"Authorization", "Bearer " + token}};
    auto res = cli.Patch("/aonx/v1/session", headers, "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    auto body = nlohmann::json::parse(res->body);
    EXPECT_EQ(body["token"], token);
    EXPECT_TRUE(body.contains("expires_at"));
    auto expires_at = body["expires_at"].get<std::string>();
    EXPECT_NE(expires_at.find('T'), std::string::npos);
    EXPECT_NE(expires_at.find('Z'), std::string::npos);
}

TEST_F(NXEndpointTest, SessionRenewOmitsExpiresAtWhenTTLZero) {
    nx_->set_session_ttl_seconds(0);
    auto token = create_session();

    auto cli = client();
    http::Headers headers = {{"Authorization", "Bearer " + token}};
    auto res = cli.Patch("/aonx/v1/session", headers, "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    auto body = nlohmann::json::parse(res->body);
    EXPECT_EQ(body["token"], token);
    EXPECT_FALSE(body.contains("expires_at"));
}

TEST_F(NXEndpointTest, SessionCreateReturnsUserAndExpiresAt) {
    nx_->set_session_ttl_seconds(600);
    auto cli = client();
    auto res = cli.Post("/aonx/v1/session", R"({"client_name":"AO-SDL","client_version":"3.0","hdid":"hwid_test"})",
                        "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 201);

    auto body = nlohmann::json::parse(res->body);
    EXPECT_TRUE(body.contains("token"));
    EXPECT_TRUE(body.contains("expires_at"));
    EXPECT_TRUE(body.contains("user"));

    auto& user = body["user"];
    EXPECT_TRUE(user.contains("id"));
    EXPECT_FALSE(user["id"].get<std::string>().empty());
    EXPECT_TRUE(user.contains("display_name"));
    EXPECT_EQ(user["display_name"], "AO-SDL");
    EXPECT_TRUE(user.contains("roles"));
    EXPECT_TRUE(user["roles"].is_array());
    EXPECT_FALSE(user["roles"].empty());
}

TEST_F(NXEndpointTest, SessionCreateOmitsExpiresAtWhenTTLZero) {
    nx_->set_session_ttl_seconds(0);
    auto cli = client();
    auto res = cli.Post("/aonx/v1/session", R"({"client_name":"test","client_version":"1.0","hdid":"abc"})",
                        "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 201);

    auto body = nlohmann::json::parse(res->body);
    EXPECT_TRUE(body.contains("token"));
    EXPECT_FALSE(body.contains("expires_at"));
    EXPECT_TRUE(body.contains("user"));
}

TEST_F(NXEndpointTest, SessionRenewRequiresAuth) {
    auto cli = client();
    auto res = cli.Patch("/aonx/v1/session", "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST_F(NXEndpointTest, ServerMotdReturnsMessage) {
    nx_->set_motd("Welcome to the courtroom!");

    auto cli = client();
    auto res = cli.Get("/aonx/v1/server");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(nlohmann::json::parse(res->body)["motd"], "Welcome to the courtroom!");
}

TEST_F(NXEndpointTest, ServerMotdReturnsEmptyWhenUnset) {
    auto cli = client();
    auto res = cli.Get("/aonx/v1/server");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(nlohmann::json::parse(res->body)["motd"], "");
}

TEST_F(NXEndpointTest, ServerNoAuthRequired) {
    nx_->set_motd("Test MOTD");

    auto cli = client();
    auto res = cli.Get("/aonx/v1/server");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
}

// -- Phase 3: Character endpoint tests ---------------------------------------

TEST_F(NXEndpointTest, CharacterListReturnsAll) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto res = cli.Get("/aonx/v1/characters", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    auto body = nlohmann::json::parse(res->body);
    ASSERT_TRUE(body.contains("characters"));
    auto& chars = body["characters"];
    EXPECT_EQ(chars.size(), 3);
    for (auto& c : chars) {
        EXPECT_TRUE(c.contains("char_id"));
        EXPECT_TRUE(c.contains("available"));
        EXPECT_TRUE(c["available"].get<bool>());
    }
}

TEST_F(NXEndpointTest, CharacterListShowsTaken) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    // Select the first character
    auto list_res = cli.Get("/aonx/v1/characters", h);
    auto chars = nlohmann::json::parse(list_res->body)["characters"];
    std::string first_id = chars[0]["char_id"].get<std::string>();

    cli.Post("/aonx/v1/characters/select", h, nlohmann::json({{"char_id", first_id}}).dump(), "application/json");

    // Re-list — the selected character should be taken
    auto res = cli.Get("/aonx/v1/characters", h);
    auto updated = nlohmann::json::parse(res->body)["characters"];
    bool found_taken = false;
    for (auto& c : updated) {
        if (c["char_id"] == first_id) {
            EXPECT_FALSE(c["available"].get<bool>());
            found_taken = true;
        }
    }
    EXPECT_TRUE(found_taken);
}

TEST_F(NXEndpointTest, CharacterListRequiresAuth) {
    auto cli = client();
    auto res = cli.Get("/aonx/v1/characters");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST_F(NXEndpointTest, CharacterGetReturnsManifest) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    std::string char_id = room_.char_id_at(0);
    auto res = cli.Get("/aonx/v1/characters/" + char_id, h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    auto body = nlohmann::json::parse(res->body);
    EXPECT_EQ(body["schema"], "character");
    EXPECT_EQ(body["schema_version"], 1);
    EXPECT_EQ(body["name"], "Phoenix");
    EXPECT_TRUE(body["emotes"].is_array());
}

TEST_F(NXEndpointTest, CharacterGetNotFound) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto res = cli.Get("/aonx/v1/characters/nonexistent", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}

TEST_F(NXEndpointTest, CharacterSelectSuccess) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    std::string char_id = room_.char_id_at(0);
    auto res =
        cli.Post("/aonx/v1/characters/select", h, nlohmann::json({{"char_id", char_id}}).dump(), "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_TRUE(nlohmann::json::parse(res->body)["accepted"].get<bool>());
}

TEST_F(NXEndpointTest, CharacterSelectAlreadyTaken) {
    // Session A takes a character
    auto token_a = create_session();
    auto cli_a = client();
    http::Headers h_a = {{"Authorization", "Bearer " + token_a}};

    std::string char_id = room_.char_id_at(0);
    cli_a.Post("/aonx/v1/characters/select", h_a, nlohmann::json({{"char_id", char_id}}).dump(), "application/json");

    // Session B tries the same character
    auto token_b = create_session();
    auto cli_b = client();
    http::Headers h_b = {{"Authorization", "Bearer " + token_b}};

    auto res = cli_b.Post("/aonx/v1/characters/select", h_b, nlohmann::json({{"char_id", char_id}}).dump(),
                          "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 409);
}

TEST_F(NXEndpointTest, CharacterSelectNotFound) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto res = cli.Post("/aonx/v1/characters/select", h, R"({"char_id":"nonexistent"})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}

// -- Phase 3: Area endpoint tests --------------------------------------------

TEST_F(NXEndpointTest, AreaListReturnsAll) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto res = cli.Get("/aonx/v1/areas", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    auto body = nlohmann::json::parse(res->body);
    ASSERT_TRUE(body.contains("areas"));
    EXPECT_EQ(body["areas"].size(), 2);

    for (auto& a : body["areas"]) {
        EXPECT_TRUE(a.contains("id"));
        EXPECT_TRUE(a.contains("name"));
        EXPECT_TRUE(a.contains("path"));
        EXPECT_TRUE(a.contains("players"));
        EXPECT_TRUE(a.contains("status"));
    }
}

TEST_F(NXEndpointTest, AreaListRequiresAuth) {
    auto cli = client();
    auto res = cli.Get("/aonx/v1/areas");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST_F(NXEndpointTest, AreaGetReturnsState) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto* lobby = room_.find_area_by_name("Lobby");
    ASSERT_NE(lobby, nullptr);

    auto res = cli.Get("/aonx/v1/areas/" + lobby->id, h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    auto body = nlohmann::json::parse(res->body);
    EXPECT_EQ(body["area"]["name"], "Lobby");
    EXPECT_TRUE(body.contains("hp"));
    EXPECT_EQ(body["hp"]["defense"], 10);
    EXPECT_EQ(body["hp"]["prosecution"], 10);
    EXPECT_TRUE(body.contains("timers"));
    EXPECT_TRUE(body["timers"].is_array());
    // background and music should be null (no state set)
    EXPECT_TRUE(body["background"].is_null());
    EXPECT_TRUE(body["music"].is_null());
}

TEST_F(NXEndpointTest, AreaGetUnderscore) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    // Session defaults to first area ("Lobby")
    auto res = cli.Get("/aonx/v1/areas/_", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    auto body = nlohmann::json::parse(res->body);
    EXPECT_EQ(body["area"]["name"], "Lobby");
}

TEST_F(NXEndpointTest, AreaGetNotFound) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto res = cli.Get("/aonx/v1/areas/nonexistent", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}

TEST_F(NXEndpointTest, AreaPlayersInArea) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto* lobby = room_.find_area_by_name("Lobby");
    ASSERT_NE(lobby, nullptr);

    auto res = cli.Get("/aonx/v1/areas/" + lobby->id + "/players", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    auto body = nlohmann::json::parse(res->body);
    ASSERT_TRUE(body.contains("players"));
    // The session we created should be in the Lobby
    EXPECT_GE(body["players"].size(), 1);
}

TEST_F(NXEndpointTest, AreaPlayersEmpty) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    // Courtroom 1 should have no players (session defaults to Lobby)
    auto* cr1 = room_.find_area_by_name("Courtroom 1");
    ASSERT_NE(cr1, nullptr);

    auto res = cli.Get("/aonx/v1/areas/" + cr1->id + "/players", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    auto body = nlohmann::json::parse(res->body);
    EXPECT_EQ(body["players"].size(), 0);
}

// -- Phase 4: Chat & Area Join endpoint tests (#91) --------------------------

TEST_F(NXEndpointTest, AreaJoinSuccess) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto* cr1 = room_.find_area_by_name("Courtroom 1");
    ASSERT_NE(cr1, nullptr);

    auto res = cli.Post("/aonx/v1/areas/" + cr1->id + "/join", h, "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_TRUE(nlohmann::json::parse(res->body)["accepted"].get<bool>());

    // Verify session moved to the new area
    auto* session = room_.find_session_by_token(token);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->area, "Courtroom 1");
}

TEST_F(NXEndpointTest, AreaJoinLocked) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto* cr1 = room_.find_area_by_name("Courtroom 1");
    ASSERT_NE(cr1, nullptr);
    cr1->lock_mode = AreaLockMode::LOCKED;

    auto res = cli.Post("/aonx/v1/areas/" + cr1->id + "/join", h, "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 403);
}

TEST_F(NXEndpointTest, AreaJoinNotFound) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto res = cli.Post("/aonx/v1/areas/nonexistent/join", h, "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}

TEST_F(NXEndpointTest, AreaJoinRejectsUnderscore) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto res = cli.Post("/aonx/v1/areas/_/join", h, "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

TEST_F(NXEndpointTest, AreaJoinRejectsStar) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto res = cli.Post("/aonx/v1/areas/*/join", h, "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

TEST_F(NXEndpointTest, AreaJoinRequiresAuth) {
    auto cli = client();
    auto* cr1 = room_.find_area_by_name("Courtroom 1");
    ASSERT_NE(cr1, nullptr);

    auto res = cli.Post("/aonx/v1/areas/" + cr1->id + "/join", "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST_F(NXEndpointTest, OocSendSuccess) {
    // Capture broadcasts to verify dispatch.
    std::vector<OOCEvent> captured;
    room_.add_ooc_broadcast([&](const std::string&, const OOCEvent& evt) { captured.push_back(evt); });

    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto* lobby = room_.find_area_by_name("Lobby");
    ASSERT_NE(lobby, nullptr);

    auto res = cli.Post("/aonx/v1/areas/" + lobby->id + "/ooc", h, R"({"name":"TestUser","message":"Hello world"})",
                        "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_TRUE(nlohmann::json::parse(res->body)["accepted"].get<bool>());

    // The NXServer broadcast stub fires first, then our capture callback.
    ASSERT_FALSE(captured.empty());
    EXPECT_EQ(captured.back().area, "Lobby");
    EXPECT_EQ(captured.back().action.name, "TestUser");
    EXPECT_EQ(captured.back().action.message, "Hello world");
}

TEST_F(NXEndpointTest, OocSendCurrentArea) {
    std::vector<OOCEvent> captured;
    room_.add_ooc_broadcast([&](const std::string&, const OOCEvent& evt) { captured.push_back(evt); });

    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    // Session defaults to Lobby; send to "_" (current area).
    auto res = cli.Post("/aonx/v1/areas/_/ooc", h, R"({"name":"Player","message":"test"})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    ASSERT_FALSE(captured.empty());
    EXPECT_EQ(captured.back().area, "Lobby");
}

TEST_F(NXEndpointTest, OocSendAllAreas) {
    std::vector<OOCEvent> captured;
    room_.add_ooc_broadcast([&](const std::string&, const OOCEvent& evt) { captured.push_back(evt); });

    auto token = create_session();
    room_.find_session_by_token(token)->moderator = true;
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto res =
        cli.Post("/aonx/v1/areas/*/ooc", h, R"({"name":"Admin","message":"server message"})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    // Should broadcast once per area (Lobby + Courtroom 1 = 2).
    // NXServer also has a callback, so total = 2 areas * 2 callbacks = 4.
    // We only count our captures.
    EXPECT_EQ(captured.size(), 2u);
    std::set<std::string> areas;
    for (auto& evt : captured)
        areas.insert(evt.area);
    EXPECT_TRUE(areas.count("Lobby"));
    EXPECT_TRUE(areas.count("Courtroom 1"));
}

TEST_F(NXEndpointTest, OocSendMissingBody) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto* lobby = room_.find_area_by_name("Lobby");
    ASSERT_NE(lobby, nullptr);

    auto res = cli.Post("/aonx/v1/areas/" + lobby->id + "/ooc", h, "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

TEST_F(NXEndpointTest, OocSendRequiresAuth) {
    auto cli = client();
    auto* lobby = room_.find_area_by_name("Lobby");
    ASSERT_NE(lobby, nullptr);

    auto res =
        cli.Post("/aonx/v1/areas/" + lobby->id + "/ooc", R"({"name":"Anon","message":"hi"})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST_F(NXEndpointTest, IcSendSuccess) {
    std::vector<ICEvent> captured;
    room_.add_ic_broadcast([&](const std::string&, const ICEvent& evt) { captured.push_back(evt); });

    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto* lobby = room_.find_area_by_name("Lobby");
    ASSERT_NE(lobby, nullptr);

    nlohmann::json ic_body = {
        {"text", {{{"id", "t0"}, {"content", "Hold it!"}, {"showname", "Phoenix"}, {"on", "start"}}}},
    };

    auto res = cli.Post("/aonx/v1/areas/" + lobby->id + "/ic", h, ic_body.dump(), "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_TRUE(nlohmann::json::parse(res->body)["accepted"].get<bool>());

    ASSERT_FALSE(captured.empty());
    EXPECT_EQ(captured.back().area, "Lobby");
    EXPECT_EQ(captured.back().action.message, "Hold it!");
    EXPECT_EQ(captured.back().action.showname, "Phoenix");
}

TEST_F(NXEndpointTest, IcSendCurrentArea) {
    std::vector<ICEvent> captured;
    room_.add_ic_broadcast([&](const std::string&, const ICEvent& evt) { captured.push_back(evt); });

    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    nlohmann::json ic_body = {
        {"text", {{{"id", "t0"}, {"content", "Test"}, {"on", "start"}}}},
    };

    auto res = cli.Post("/aonx/v1/areas/_/ic", h, ic_body.dump(), "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    ASSERT_FALSE(captured.empty());
    EXPECT_EQ(captured.back().area, "Lobby");
}

TEST_F(NXEndpointTest, IcSendAllAreas) {
    std::vector<ICEvent> captured;
    room_.add_ic_broadcast([&](const std::string&, const ICEvent& evt) { captured.push_back(evt); });

    auto token = create_session();
    room_.find_session_by_token(token)->moderator = true;
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    nlohmann::json ic_body = {
        {"text", {{{"id", "t0"}, {"content", "Objection!"}, {"on", "start"}}}},
    };

    auto res = cli.Post("/aonx/v1/areas/*/ic", h, ic_body.dump(), "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    EXPECT_EQ(captured.size(), 2u);
}

TEST_F(NXEndpointTest, IcSendMissingBody) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto* lobby = room_.find_area_by_name("Lobby");
    ASSERT_NE(lobby, nullptr);

    auto res = cli.Post("/aonx/v1/areas/" + lobby->id + "/ic", h, "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

TEST_F(NXEndpointTest, IcSendRequiresAuth) {
    auto cli = client();
    auto* lobby = room_.find_area_by_name("Lobby");
    ASSERT_NE(lobby, nullptr);

    auto res = cli.Post("/aonx/v1/areas/" + lobby->id + "/ic", R"({"text":[{"id":"t0","content":"hi","on":"start"}]})",
                        "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST_F(NXEndpointTest, IcSendFlattensObjectAndAudio) {
    std::vector<ICEvent> captured;
    room_.add_ic_broadcast([&](const std::string&, const ICEvent& evt) { captured.push_back(evt); });

    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto* lobby = room_.find_area_by_name("Lobby");
    ASSERT_NE(lobby, nullptr);

    nlohmann::json ic_body = {
        {"text",
         {{{"id", "t0"},
           {"content", "Take that!"},
           {"showname", "Edgeworth"},
           {"additive", true},
           {"immediate", true},
           {"on", "start"}}}},
        {"objects",
         {{{"id", "def"},
           {"z", 0},
           {"visible", true},
           {"initial_state", "pointing"},
           {"transform", {{"scale_x", -1.0}}}}}},
        {"audio", {{{"asset", "sfx-objection"}, {"delay_ms", 100}, {"on", "start"}}}},
        {"effects", {{{"shader", "realization-flash"}, {"on", "start"}}}},
    };

    auto res = cli.Post("/aonx/v1/areas/" + lobby->id + "/ic", h, ic_body.dump(), "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    ASSERT_FALSE(captured.empty());
    auto& action = captured.back().action;
    EXPECT_EQ(action.message, "Take that!");
    EXPECT_EQ(action.showname, "Edgeworth");
    EXPECT_TRUE(action.additive);
    EXPECT_TRUE(action.immediate);
    EXPECT_EQ(action.side, "def");
    EXPECT_EQ(action.emote, "pointing");
    EXPECT_TRUE(action.flip);
    EXPECT_EQ(action.sfx_name, "sfx-objection");
    EXPECT_EQ(action.sfx_delay, 100);
    EXPECT_TRUE(action.realization);
}

TEST_F(NXEndpointTest, IcSendAllAreasDeniedWithoutModerator) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    nlohmann::json ic_body = {
        {"text", {{{"id", "t0"}, {"content", "spam"}, {"on", "start"}}}},
    };

    auto res = cli.Post("/aonx/v1/areas/*/ic", h, ic_body.dump(), "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 403);
}

TEST_F(NXEndpointTest, OocSendAllAreasDeniedWithoutModerator) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto res = cli.Post("/aonx/v1/areas/*/ooc", h, R"({"name":"Player","message":"spam"})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 403);
}

TEST_F(NXEndpointTest, IcSendNotFoundArea) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    nlohmann::json ic_body = {
        {"text", {{{"id", "t0"}, {"content", "hello"}, {"on", "start"}}}},
    };

    auto res = cli.Post("/aonx/v1/areas/nonexistent/ic", h, ic_body.dump(), "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}

TEST_F(NXEndpointTest, OocSendNotFoundArea) {
    auto token = create_session();
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto res = cli.Post("/aonx/v1/areas/nonexistent/ooc", h, R"({"name":"Player","message":"hi"})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}

// ==========================================================================
// AONX API Integration Tests
//
// Full lifecycle tests exercised over real HTTP loopback sockets:
// session management, player counts, character selection, area navigation,
// auth semantics, and edge cases around special identifiers.
// ==========================================================================

// -- Session lifecycle -------------------------------------------------------

TEST_F(NXEndpointTest, SessionLifecycle_CreateReturnsToken) {
    auto cli = client();
    auto res = cli.Post("/aonx/v1/session", R"({"client_name":"IntegTest","client_version":"2.0","hdid":"hw123"})",
                        "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 201);

    auto body = nlohmann::json::parse(res->body);
    EXPECT_TRUE(body.contains("token"));
    EXPECT_FALSE(body["token"].get<std::string>().empty());
}

TEST_F(NXEndpointTest, SessionLifecycle_PlayerCountIncrements) {
    auto cli = client();

    // Baseline: 0 players
    auto res0 = cli.Get("/aonx/v1/server");
    ASSERT_TRUE(res0);
    EXPECT_EQ(nlohmann::json::parse(res0->body)["online"], 0);

    // Create first session
    create_session();
    auto res1 = cli.Get("/aonx/v1/server");
    ASSERT_TRUE(res1);
    EXPECT_EQ(nlohmann::json::parse(res1->body)["online"], 1);

    // Create second session
    create_session();
    auto res2 = cli.Get("/aonx/v1/server");
    ASSERT_TRUE(res2);
    EXPECT_EQ(nlohmann::json::parse(res2->body)["online"], 2);
}

TEST_F(NXEndpointTest, SessionLifecycle_DeleteDecrementsPlayerCount) {
    auto cli = client();
    auto token = create_session();

    // Verify 1 player
    auto res1 = cli.Get("/aonx/v1/server");
    EXPECT_EQ(nlohmann::json::parse(res1->body)["online"], 1);

    // Delete the session
    http::Headers h = {{"Authorization", "Bearer " + token}};
    auto del = cli.Delete("/aonx/v1/session", h);
    ASSERT_TRUE(del);
    EXPECT_EQ(del->status, 204);

    // Player count should be back to 0
    auto res2 = cli.Get("/aonx/v1/server");
    EXPECT_EQ(nlohmann::json::parse(res2->body)["online"], 0);
}

TEST_F(NXEndpointTest, SessionLifecycle_DeletedTokenBecomesInvalid) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    // Delete the session
    auto del = cli.Delete("/aonx/v1/session", h);
    ASSERT_TRUE(del);
    EXPECT_EQ(del->status, 204);

    // Using the same token for an authed endpoint should now fail
    auto res = cli.Get("/aonx/v1/characters", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST_F(NXEndpointTest, SessionLifecycle_RenewRefreshesToken) {
    nx_->set_session_ttl_seconds(600);
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto res = cli.Patch("/aonx/v1/session", h, "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    auto body = nlohmann::json::parse(res->body);
    EXPECT_EQ(body["token"], token);
    EXPECT_TRUE(body.contains("expires_at"));
    // expires_at should be an ISO 8601 timestamp
    auto expires = body["expires_at"].get<std::string>();
    EXPECT_NE(expires.find('T'), std::string::npos);
    EXPECT_NE(expires.find('Z'), std::string::npos);
}

TEST_F(NXEndpointTest, SessionLifecycle_MaxPlayersEnforced) {
    room_.max_players = 2;
    auto cli = client();

    // Fill up
    auto first_token = create_session();
    create_session();
    EXPECT_EQ(nlohmann::json::parse(cli.Get("/aonx/v1/server")->body)["online"], 2);
    EXPECT_EQ(nlohmann::json::parse(cli.Get("/aonx/v1/server")->body)["max"], 2);

    // Third session should be rejected
    auto res = cli.Post("/aonx/v1/session", R"({"client_name":"overflow","client_version":"1.0","hdid":"x"})",
                        "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 503);
    EXPECT_EQ(nlohmann::json::parse(res->body)["reason"], "Server is full");

    // Delete one, then retry
    http::Headers h1 = {{"Authorization", "Bearer " + first_token}};
    cli.Delete("/aonx/v1/session", h1);

    auto res2 = cli.Post("/aonx/v1/session", R"({"client_name":"latecomer","client_version":"1.0","hdid":"y"})",
                         "application/json");
    ASSERT_TRUE(res2);
    EXPECT_EQ(res2->status, 201);
}

TEST_F(NXEndpointTest, SessionLifecycle_MultipleSessionsIndependent) {
    auto cli = client();
    auto token_a = create_session();
    auto token_b = create_session();

    // Delete session A
    http::Headers ha = {{"Authorization", "Bearer " + token_a}};
    cli.Delete("/aonx/v1/session", ha);

    // Session B should still work
    http::Headers hb = {{"Authorization", "Bearer " + token_b}};
    auto res = cli.Get("/aonx/v1/characters", hb);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    // Session A should be gone
    auto res2 = cli.Get("/aonx/v1/characters", ha);
    ASSERT_TRUE(res2);
    EXPECT_EQ(res2->status, 401);
}

// -- Session create validation -----------------------------------------------

TEST_F(NXEndpointTest, SessionCreate_MissingBody) {
    auto cli = client();
    auto res = cli.Post("/aonx/v1/session", "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

TEST_F(NXEndpointTest, SessionCreate_MissingRequiredFields) {
    auto cli = client();
    // Missing client_version and hdid
    auto res = cli.Post("/aonx/v1/session", R"({"client_name":"test"})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

TEST_F(NXEndpointTest, SessionCreate_EmptyJsonObject) {
    auto cli = client();
    auto res = cli.Post("/aonx/v1/session", R"({})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

// -- Auth edge cases ---------------------------------------------------------

TEST_F(NXEndpointTest, Auth_MissingAuthorizationHeader) {
    auto cli = client();
    auto res = cli.Get("/aonx/v1/characters");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST_F(NXEndpointTest, Auth_MalformedBearerPrefix) {
    auto cli = client();
    // "Token" instead of "Bearer"
    http::Headers h = {{"Authorization", "Token abc123"}};
    auto res = cli.Get("/aonx/v1/characters", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST_F(NXEndpointTest, Auth_EmptyBearerToken) {
    auto cli = client();
    // "Bearer " with nothing after it
    http::Headers h = {{"Authorization", "Bearer "}};
    auto res = cli.Get("/aonx/v1/characters", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST_F(NXEndpointTest, Auth_GarbageToken) {
    auto cli = client();
    http::Headers h = {{"Authorization", "Bearer not-a-real-token-at-all"}};
    auto res = cli.Get("/aonx/v1/characters", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST_F(NXEndpointTest, Auth_TokensAreUniquePerSession) {
    auto token_a = create_session();
    auto token_b = create_session();
    EXPECT_NE(token_a, token_b);
}

// -- Character selection lifecycle -------------------------------------------

TEST_F(NXEndpointTest, CharacterSelect_FreesOnSessionDelete) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    // Select a character
    std::string char_id = room_.char_id_at(0);
    auto sel =
        cli.Post("/aonx/v1/characters/select", h, nlohmann::json({{"char_id", char_id}}).dump(), "application/json");
    ASSERT_TRUE(sel);
    EXPECT_EQ(sel->status, 200);

    // Verify it's taken
    auto list1 = cli.Get("/aonx/v1/characters", h);
    auto chars1 = nlohmann::json::parse(list1->body)["characters"];
    for (auto& c : chars1) {
        if (c["char_id"] == char_id) {
            EXPECT_FALSE(c["available"].get<bool>());
        }
    }

    // Delete session — character should be freed
    cli.Delete("/aonx/v1/session", h);

    // Create a new session and verify character is available again
    auto token2 = create_session();
    http::Headers h2 = {{"Authorization", "Bearer " + token2}};
    auto list2 = cli.Get("/aonx/v1/characters", h2);
    auto chars2 = nlohmann::json::parse(list2->body)["characters"];
    for (auto& c : chars2) {
        if (c["char_id"] == char_id) {
            EXPECT_TRUE(c["available"].get<bool>());
        }
    }
}

TEST_F(NXEndpointTest, CharacterSelect_SameSessionCanReselect) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    // Select first character
    std::string char0 = room_.char_id_at(0);
    auto res1 =
        cli.Post("/aonx/v1/characters/select", h, nlohmann::json({{"char_id", char0}}).dump(), "application/json");
    ASSERT_TRUE(res1);
    EXPECT_EQ(res1->status, 200);

    // Select a different character — should succeed (releases the first)
    std::string char1 = room_.char_id_at(1);
    auto res2 =
        cli.Post("/aonx/v1/characters/select", h, nlohmann::json({{"char_id", char1}}).dump(), "application/json");
    ASSERT_TRUE(res2);
    EXPECT_EQ(res2->status, 200);

    // First character should be available again
    auto list = cli.Get("/aonx/v1/characters", h);
    auto chars = nlohmann::json::parse(list->body)["characters"];
    for (auto& c : chars) {
        if (c["char_id"] == char0) {
            EXPECT_TRUE(c["available"].get<bool>());
        }
        if (c["char_id"] == char1) {
            EXPECT_FALSE(c["available"].get<bool>());
        }
    }
}

TEST_F(NXEndpointTest, CharacterSelect_MissingCharIdField) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto res = cli.Post("/aonx/v1/characters/select", h, R"({"wrong":"field"})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

TEST_F(NXEndpointTest, CharacterGet_AllCharactersHaveManifest) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    // Get the character list
    auto list = cli.Get("/aonx/v1/characters", h);
    auto chars = nlohmann::json::parse(list->body)["characters"];

    // Verify each character has a valid manifest
    for (auto& c : chars) {
        auto cid = c["char_id"].get<std::string>();
        auto detail = cli.Get("/aonx/v1/characters/" + cid, h);
        ASSERT_TRUE(detail);
        EXPECT_EQ(detail->status, 200);

        auto manifest = nlohmann::json::parse(detail->body);
        EXPECT_EQ(manifest["schema"], "character");
        EXPECT_EQ(manifest["schema_version"], 1);
        EXPECT_TRUE(manifest.contains("name"));
        EXPECT_TRUE(manifest.contains("emotes"));
        EXPECT_TRUE(manifest["emotes"].is_array());
    }
}

// -- Area navigation lifecycle -----------------------------------------------

TEST_F(NXEndpointTest, AreaJoin_SessionStartsInFirstArea) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    // "_" should resolve to the first area (Lobby) since session defaults there
    auto res = cli.Get("/aonx/v1/areas/_", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(nlohmann::json::parse(res->body)["area"]["name"], "Lobby");
}

TEST_F(NXEndpointTest, AreaJoin_MoveUpdatesCurrentArea) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    // Move to Courtroom 1
    auto* cr1 = room_.find_area_by_name("Courtroom 1");
    ASSERT_NE(cr1, nullptr);

    auto join = cli.Post("/aonx/v1/areas/" + cr1->id + "/join", h, "", "application/json");
    ASSERT_TRUE(join);
    EXPECT_EQ(join->status, 200);

    // "_" should now resolve to Courtroom 1
    auto res = cli.Get("/aonx/v1/areas/_", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(nlohmann::json::parse(res->body)["area"]["name"], "Courtroom 1");
}

TEST_F(NXEndpointTest, AreaJoin_PlayerCountsReflectMovement) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto* lobby = room_.find_area_by_name("Lobby");
    auto* cr1 = room_.find_area_by_name("Courtroom 1");
    ASSERT_NE(lobby, nullptr);
    ASSERT_NE(cr1, nullptr);

    // Initially: 1 player in Lobby, 0 in Courtroom 1
    auto lobby_players = cli.Get("/aonx/v1/areas/" + lobby->id + "/players", h);
    EXPECT_EQ(nlohmann::json::parse(lobby_players->body)["players"].size(), 1);
    auto cr1_players = cli.Get("/aonx/v1/areas/" + cr1->id + "/players", h);
    EXPECT_EQ(nlohmann::json::parse(cr1_players->body)["players"].size(), 0);

    // Move to Courtroom 1
    cli.Post("/aonx/v1/areas/" + cr1->id + "/join", h, "", "application/json");

    // Now: 0 in Lobby, 1 in Courtroom 1
    lobby_players = cli.Get("/aonx/v1/areas/" + lobby->id + "/players", h);
    EXPECT_EQ(nlohmann::json::parse(lobby_players->body)["players"].size(), 0);
    cr1_players = cli.Get("/aonx/v1/areas/" + cr1->id + "/players", h);
    EXPECT_EQ(nlohmann::json::parse(cr1_players->body)["players"].size(), 1);
}

TEST_F(NXEndpointTest, AreaJoin_MultiplePlayersInSameArea) {
    auto cli = client();
    auto token_a = create_session();
    create_session();

    auto* lobby = room_.find_area_by_name("Lobby");
    ASSERT_NE(lobby, nullptr);

    // Both sessions start in Lobby
    http::Headers ha = {{"Authorization", "Bearer " + token_a}};
    auto res = cli.Get("/aonx/v1/areas/" + lobby->id + "/players", ha);
    EXPECT_EQ(nlohmann::json::parse(res->body)["players"].size(), 2);
}

TEST_F(NXEndpointTest, AreaJoin_RejoinSameAreaIsIdempotent) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto* lobby = room_.find_area_by_name("Lobby");
    ASSERT_NE(lobby, nullptr);

    // Join Lobby again (already there) — should succeed
    auto res = cli.Post("/aonx/v1/areas/" + lobby->id + "/join", h, "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    // Still in Lobby
    auto area_res = cli.Get("/aonx/v1/areas/_", h);
    EXPECT_EQ(nlohmann::json::parse(area_res->body)["area"]["name"], "Lobby");
}

// -- Area special identifiers (_ and *) --------------------------------------

TEST_F(NXEndpointTest, AreaSpecial_UnderscoreResolvesToCurrentArea) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    // Default area is Lobby
    auto res = cli.Get("/aonx/v1/areas/_", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(nlohmann::json::parse(res->body)["area"]["name"], "Lobby");

    // Move to Courtroom 1
    auto* cr1 = room_.find_area_by_name("Courtroom 1");
    cli.Post("/aonx/v1/areas/" + cr1->id + "/join", h, "", "application/json");

    // _ now resolves to Courtroom 1
    auto res2 = cli.Get("/aonx/v1/areas/_", h);
    EXPECT_EQ(nlohmann::json::parse(res2->body)["area"]["name"], "Courtroom 1");
}

TEST_F(NXEndpointTest, AreaSpecial_UnderscorePlayersShowsCurrentArea) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    // Players in "_" should list players in the session's current area
    auto res = cli.Get("/aonx/v1/areas/_/players", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_GE(nlohmann::json::parse(res->body)["players"].size(), 1);
}

TEST_F(NXEndpointTest, AreaSpecial_UnderscoreCannotBeJoined) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto res = cli.Post("/aonx/v1/areas/_/join", h, "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

TEST_F(NXEndpointTest, AreaSpecial_StarCannotBeJoined) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto res = cli.Post("/aonx/v1/areas/*/join", h, "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

TEST_F(NXEndpointTest, AreaSpecial_StarOocRequiresModerator) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    // Non-moderator should be rejected
    auto res = cli.Post("/aonx/v1/areas/*/ooc", h, R"({"name":"Player","message":"broadcast"})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 403);
}

TEST_F(NXEndpointTest, AreaSpecial_StarOocSucceedsForModerator) {
    std::vector<OOCEvent> captured;
    room_.add_ooc_broadcast([&](const std::string&, const OOCEvent& evt) { captured.push_back(evt); });

    auto cli = client();
    auto token = create_session();
    room_.find_session_by_token(token)->moderator = true;
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto res =
        cli.Post("/aonx/v1/areas/*/ooc", h, R"({"name":"Admin","message":"server announcement"})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    // Should broadcast to all areas (Lobby + Courtroom 1)
    EXPECT_EQ(captured.size(), 2u);
}

TEST_F(NXEndpointTest, AreaSpecial_StarIcRequiresModerator) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    nlohmann::json ic_body = {
        {"text", {{{"id", "t0"}, {"content", "spam"}, {"on", "start"}}}},
    };

    auto res = cli.Post("/aonx/v1/areas/*/ic", h, ic_body.dump(), "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 403);
}

TEST_F(NXEndpointTest, AreaSpecial_UnderscoreOocSendsToCurrentArea) {
    std::vector<OOCEvent> captured;
    room_.add_ooc_broadcast([&](const std::string&, const OOCEvent& evt) { captured.push_back(evt); });

    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    // Move to Courtroom 1
    auto* cr1 = room_.find_area_by_name("Courtroom 1");
    cli.Post("/aonx/v1/areas/" + cr1->id + "/join", h, "", "application/json");

    // Send OOC to "_" — should go to Courtroom 1
    auto res = cli.Post("/aonx/v1/areas/_/ooc", h, R"({"name":"Player","message":"hello"})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    ASSERT_FALSE(captured.empty());
    EXPECT_EQ(captured.back().area, "Courtroom 1");
}

TEST_F(NXEndpointTest, AreaSpecial_UnderscoreIcSendsToCurrentArea) {
    std::vector<ICEvent> captured;
    room_.add_ic_broadcast([&](const std::string&, const ICEvent& evt) { captured.push_back(evt); });

    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    // Move to Courtroom 1
    auto* cr1 = room_.find_area_by_name("Courtroom 1");
    cli.Post("/aonx/v1/areas/" + cr1->id + "/join", h, "", "application/json");

    nlohmann::json ic_body = {
        {"text", {{{"id", "t0"}, {"content", "Objection!"}, {"on", "start"}}}},
    };
    auto res = cli.Post("/aonx/v1/areas/_/ic", h, ic_body.dump(), "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    ASSERT_FALSE(captured.empty());
    EXPECT_EQ(captured.back().area, "Courtroom 1");
}

// -- Area list and state -----------------------------------------------------

TEST_F(NXEndpointTest, AreaList_ContainsAllAreas) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto res = cli.Get("/aonx/v1/areas", h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    auto body = nlohmann::json::parse(res->body);
    auto& areas = body["areas"];
    EXPECT_EQ(areas.size(), 2);

    // Collect names
    std::set<std::string> names;
    for (auto& a : areas) {
        names.insert(a["name"].get<std::string>());
        EXPECT_TRUE(a.contains("id"));
        EXPECT_TRUE(a.contains("path"));
        EXPECT_TRUE(a.contains("players"));
        EXPECT_TRUE(a.contains("status"));
        EXPECT_TRUE(a.contains("locked"));
    }
    EXPECT_TRUE(names.count("Lobby"));
    EXPECT_TRUE(names.count("Courtroom 1"));
}

TEST_F(NXEndpointTest, AreaList_PlayerCountsAreAccurate) {
    auto cli = client();
    auto token_a = create_session();
    auto token_b = create_session();
    http::Headers ha = {{"Authorization", "Bearer " + token_a}};
    http::Headers hb = {{"Authorization", "Bearer " + token_b}};

    // Move session B to Courtroom 1
    auto* cr1 = room_.find_area_by_name("Courtroom 1");
    cli.Post("/aonx/v1/areas/" + cr1->id + "/join", hb, "", "application/json");

    // Area list should show 1 in each area
    auto res = cli.Get("/aonx/v1/areas", ha);
    auto areas = nlohmann::json::parse(res->body)["areas"];
    for (auto& a : areas) {
        EXPECT_EQ(a["players"].get<int>(), 1) << "Area: " << a["name"];
    }
}

TEST_F(NXEndpointTest, AreaGet_LockedAreaShowsLocked) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto* cr1 = room_.find_area_by_name("Courtroom 1");
    cr1->lock_mode = AreaLockMode::LOCKED;

    // The area list should report locked
    auto res = cli.Get("/aonx/v1/areas", h);
    auto areas = nlohmann::json::parse(res->body)["areas"];
    for (auto& a : areas) {
        if (a["name"] == "Courtroom 1") {
            EXPECT_TRUE(a["locked"].get<bool>());
        }
    }
}

TEST_F(NXEndpointTest, AreaGet_DetailIncludesHPAndTimers) {
    auto cli = client();
    auto token = create_session();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    auto* lobby = room_.find_area_by_name("Lobby");
    auto res = cli.Get("/aonx/v1/areas/" + lobby->id, h);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    auto body = nlohmann::json::parse(res->body);
    EXPECT_TRUE(body.contains("hp"));
    EXPECT_EQ(body["hp"]["defense"], 10);
    EXPECT_EQ(body["hp"]["prosecution"], 10);
    EXPECT_TRUE(body.contains("timers"));
    EXPECT_TRUE(body["timers"].is_array());
}

// -- Server info endpoints ---------------------------------------------------

TEST_F(NXEndpointTest, Server_ReturnsAllFields) {
    nx_->set_motd("Hello world");
    room_.max_players = 42;

    auto cli = client();
    auto res = cli.Get("/aonx/v1/server");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    auto body = nlohmann::json::parse(res->body);
    EXPECT_TRUE(body.contains("software"));
    EXPECT_TRUE(body.contains("version"));
    EXPECT_TRUE(body.contains("name"));
    EXPECT_TRUE(body.contains("description"));
    EXPECT_TRUE(body.contains("motd"));
    EXPECT_TRUE(body.contains("online"));
    EXPECT_TRUE(body.contains("max"));
    EXPECT_EQ(body["motd"], "Hello world");
    EXPECT_EQ(body["max"], 42);
    EXPECT_EQ(body["online"], 0);
}

// -- Full lifecycle: connect → pick char → join area → chat → leave ----------

TEST_F(NXEndpointTest, FullLifecycle_ConnectSelectJoinChatLeave) {
    std::vector<ICEvent> ic_captured;
    std::vector<OOCEvent> ooc_captured;
    room_.add_ic_broadcast([&](const std::string&, const ICEvent& evt) { ic_captured.push_back(evt); });
    room_.add_ooc_broadcast([&](const std::string&, const OOCEvent& evt) { ooc_captured.push_back(evt); });

    auto cli = client();

    // 1. Create session
    auto res1 = cli.Post("/aonx/v1/session", R"({"client_name":"AO-SDL","client_version":"3.0","hdid":"hwid_abc"})",
                         "application/json");
    ASSERT_TRUE(res1);
    EXPECT_EQ(res1->status, 201);
    auto token = nlohmann::json::parse(res1->body)["token"].get<std::string>();
    http::Headers h = {{"Authorization", "Bearer " + token}};

    // 2. List characters
    auto chars_res = cli.Get("/aonx/v1/characters", h);
    ASSERT_TRUE(chars_res);
    EXPECT_EQ(chars_res->status, 200);
    auto chars = nlohmann::json::parse(chars_res->body)["characters"];
    EXPECT_EQ(chars.size(), 3);

    // 3. Select a character
    auto char_id = chars[0]["char_id"].get<std::string>();
    auto sel =
        cli.Post("/aonx/v1/characters/select", h, nlohmann::json({{"char_id", char_id}}).dump(), "application/json");
    ASSERT_TRUE(sel);
    EXPECT_EQ(sel->status, 200);

    // 4. List areas
    auto areas_res = cli.Get("/aonx/v1/areas", h);
    auto areas = nlohmann::json::parse(areas_res->body)["areas"];
    EXPECT_EQ(areas.size(), 2);

    // 5. Join Courtroom 1
    auto* cr1 = room_.find_area_by_name("Courtroom 1");
    auto join = cli.Post("/aonx/v1/areas/" + cr1->id + "/join", h, "", "application/json");
    ASSERT_TRUE(join);
    EXPECT_EQ(join->status, 200);

    // 6. Send OOC message in current area
    auto ooc =
        cli.Post("/aonx/v1/areas/_/ooc", h, R"({"name":"Phoenix","message":"Ready for trial"})", "application/json");
    ASSERT_TRUE(ooc);
    EXPECT_EQ(ooc->status, 200);
    ASSERT_FALSE(ooc_captured.empty());
    EXPECT_EQ(ooc_captured.back().area, "Courtroom 1");
    EXPECT_EQ(ooc_captured.back().action.message, "Ready for trial");

    // 7. Send IC message
    nlohmann::json ic_body = {
        {"text", {{{"id", "t0"}, {"content", "Hold it!"}, {"showname", "Phoenix"}, {"on", "start"}}}},
    };
    auto ic = cli.Post("/aonx/v1/areas/_/ic", h, ic_body.dump(), "application/json");
    ASSERT_TRUE(ic);
    EXPECT_EQ(ic->status, 200);
    ASSERT_FALSE(ic_captured.empty());
    EXPECT_EQ(ic_captured.back().area, "Courtroom 1");
    EXPECT_EQ(ic_captured.back().action.message, "Hold it!");

    // 8. Verify player list shows our character
    auto pl = cli.Get("/aonx/v1/areas/_/players", h);
    auto players = nlohmann::json::parse(pl->body)["players"];
    EXPECT_EQ(players.size(), 1);

    // 9. Delete session
    auto del = cli.Delete("/aonx/v1/session", h);
    EXPECT_EQ(del->status, 204);

    // 10. Verify everything is cleaned up
    EXPECT_EQ(room_.session_count(), 0u);
    auto pc = cli.Get("/aonx/v1/server");
    EXPECT_EQ(nlohmann::json::parse(pc->body)["online"], 0);
}

// ---------------------------------------------------------------------------
// Per-session REST traffic counters
// ---------------------------------------------------------------------------

TEST_F(RestRouterTest, SessionTrafficCountersWithResponseBody) {
    // Authenticated endpoint returning a JSON body — should record
    // bytes_received, bytes_sent, packets_received, packets_sent.
    auto ep = std::make_unique<StubEndpoint>("POST", "/echo", true, RestResponse::json(200, {{"reply", "hello"}}));
    router_.register_endpoint(std::move(ep));

    auto session = room_.create_session(1, "aonx");
    session->session_token = "traffic_tok";
    room_.register_session_token("traffic_tok", 1);
    session->joined = true;

    router_.set_auth_func(
        [this](const std::string& token) -> ServerSession* { return room_.find_session_by_token(token); });

    start();
    auto cli = client();
    http::Headers headers = {{"Authorization", "Bearer traffic_tok"}};
    std::string body = R"({"data":"test"})";
    auto res = cli.Post("/echo", headers, body, "application/json");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    // Session should have recorded traffic
    EXPECT_GT(session->bytes_received.load(), 0u);
    EXPECT_GT(session->bytes_sent.load(), 0u);
    EXPECT_EQ(session->packets_received.load(), 1u);
    EXPECT_EQ(session->packets_sent.load(), 1u);
}

TEST_F(RestRouterTest, SessionTrafficCountersNoBody) {
    // Authenticated endpoint returning 204 (no body) — should still
    // record packets but bytes_sent stays 0.
    auto ep = std::make_unique<StubEndpoint>("DELETE", "/item", true, RestResponse{204, nullptr});
    router_.register_endpoint(std::move(ep));

    auto session = room_.create_session(1, "aonx");
    session->session_token = "del_tok";
    room_.register_session_token("del_tok", 1);
    session->joined = true;

    router_.set_auth_func(
        [this](const std::string& token) -> ServerSession* { return room_.find_session_by_token(token); });

    start();
    auto cli = client();
    http::Headers headers = {{"Authorization", "Bearer del_tok"}};
    auto res = cli.Delete("/item", headers);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 204);

    // bytes_sent should be 0 (no body), but packets should be counted
    EXPECT_EQ(session->bytes_sent.load(), 0u);
    EXPECT_EQ(session->packets_received.load(), 1u);
    EXPECT_EQ(session->packets_sent.load(), 1u);
}

// ---------------------------------------------------------------------------
// Dispatch lock stats
// ---------------------------------------------------------------------------

TEST_F(RestRouterTest, ExclusiveLockStatsRecorded) {
    // with_lock should record exclusive_acquisitions and timing
    router_.with_lock([] { /* no-op */ });

    EXPECT_EQ(router_.lock_stats.exclusive_acquisitions.load(), 1u);
    // wait_ns and hold_ns are >= 0 (could be 0 on fast machines)
    EXPECT_GE(router_.lock_stats.exclusive_hold_ns.load(), 0u);
}

TEST_F(RestRouterTest, SharedLockStatsRecorded) {
    // with_shared_lock should record shared_acquisitions and timing
    router_.with_shared_lock([] { /* no-op */ });

    EXPECT_EQ(router_.lock_stats.shared_acquisitions.load(), 1u);
    EXPECT_GE(router_.lock_stats.shared_wait_ns.load(), 0u);
}
