#include <gtest/gtest.h>

#include "game/GameRoom.h"
#include "game/ServerSession.h"
#include "net/EndpointFactory.h"
#include "net/RestEndpoint.h"
#include "net/RestRouter.h"
#include "net/nx/NXEndpoint.h"
#include "net/nx/NXServer.h"
#include "utils/Log.h"

#include <httplib.h>

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

    httplib::Client client() {
        return httplib::Client("127.0.0.1", port_);
    }

    httplib::Server http_;
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
    httplib::Headers headers = {{"Authorization", "Bearer badtoken"}};
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

    auto& session = room_.create_session(1, "aonx");
    session.session_token = "validtoken";
    session.joined = true;

    router_.set_auth_func(
        [this](const std::string& token) -> ServerSession* { return room_.find_session_by_token(token); });

    start();
    auto cli = client();
    httplib::Headers headers = {{"Authorization", "Bearer validtoken"}};
    auto res = cli.Get("/secret", headers);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    ASSERT_NE(ep_ptr->last_session, nullptr);
    EXPECT_EQ(ep_ptr->last_session->session_token, "validtoken");
}

TEST_F(RestRouterTest, AuthTouchesSession) {
    auto ep = std::make_unique<StubEndpoint>("GET", "/secret", true, RestResponse::json(200, {}));
    router_.register_endpoint(std::move(ep));

    auto& session = room_.create_session(1, "aonx");
    session.session_token = "tok";
    session.joined = true;

    // Backdate the last_activity
    session.last_activity = std::chrono::steady_clock::now() - std::chrono::seconds(100);
    auto old_activity = session.last_activity;

    router_.set_auth_func(
        [this](const std::string& token) -> ServerSession* { return room_.find_session_by_token(token); });

    start();
    auto cli = client();
    httplib::Headers headers = {{"Authorization", "Bearer tok"}};
    auto res = cli.Get("/secret", headers);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_GT(session.last_activity, old_activity);
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

// -- GameRoom session tests --------------------------------------------------

TEST(GameRoomTest, FindSessionByToken) {
    GameRoom room;
    room.areas = {"Lobby"};
    auto& s1 = room.create_session(1, "aonx");
    s1.session_token = "token_a";
    auto& s2 = room.create_session(2, "aonx");
    s2.session_token = "token_b";

    EXPECT_EQ(room.find_session_by_token("token_a"), &s1);
    EXPECT_EQ(room.find_session_by_token("token_b"), &s2);
    EXPECT_EQ(room.find_session_by_token("nonexistent"), nullptr);
}

TEST(GameRoomTest, ExpireSessionsRemovesStale) {
    GameRoom room;
    room.areas = {"Lobby"};
    room.characters = {"Phoenix"};
    room.reset_taken();

    auto& s1 = room.create_session(1, "aonx");
    s1.session_token = "active";
    s1.touch();

    auto& s2 = room.create_session(2, "aonx");
    s2.session_token = "stale";
    s2.character_id = 0;
    room.char_taken[0] = 1;
    s2.last_activity = std::chrono::steady_clock::now() - std::chrono::seconds(600);

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
    auto& s = room.create_session(1, "ao2");
    s.last_activity = std::chrono::steady_clock::now() - std::chrono::seconds(9999);

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
    httplib::Server http;
    router.bind(http);
    int port = http.bind_to_any_port("127.0.0.1");
    ASSERT_GT(port, 0);

    std::thread t([&] { http.listen_after_bind(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    httplib::Client cli("127.0.0.1", port);
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

    httplib::Server http;
    router.bind(http);
    int port = http.bind_to_any_port("127.0.0.1");
    ASSERT_GT(port, 0);

    std::thread t([&] { http.listen_after_bind(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    httplib::Client cli("127.0.0.1", port);

    // The global factory should have registered GET /aonx/v1/server/players
    auto res = cli.Get("/aonx/v1/server/players");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    auto body = nlohmann::json::parse(res->body);
    EXPECT_EQ(body["online"], 0);
    EXPECT_EQ(body["max"], 50);

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
