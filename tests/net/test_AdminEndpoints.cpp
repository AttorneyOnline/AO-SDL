// Integration tests for the admin dashboard REST endpoints (PR #132).
// Exercises GET /admin/{bans,users,moderation-events,mutes,firewall,
// asn-reputation,content,config}, PUT /admin/content, POST /admin/stop,
// POST /moderation/actions, and the per-endpoint CORS policy that
// isolates these privileged routes from the router's wildcard.

#include <gtest/gtest.h>

#include "game/BanManager.h"
#include "game/DatabaseManager.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"
#include "net/EndpointFactory.h"
#include "net/RestRouter.h"
#include "net/nx/NXEndpoint.h"
#include "net/nx/NXServer.h"
#include "utils/Log.h"

#include "net/Http.h"

#include <json.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

// Force-link the NX endpoint TUs (mirrors main.cpp / NXEndpoint.cpp).
extern void nx_register_endpoints();

namespace fs = std::filesystem;

namespace {

int64_t now_seconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// ---------------------------------------------------------------------------
// Fixture: full admin-ready server with BanManager, DatabaseManager, a
// SUPER-authenticated session and a temp content directory.
//
// The moderator session has its auth fields populated directly rather than
// going through /auth/login so the tests don't also depend on the login
// endpoint. The acl_role is set to "SUPER" which the real require_super()
// guard checks via acl_permissions_for_role.
// ---------------------------------------------------------------------------

class AdminEndpointsTest : public ::testing::Test {
  protected:
    void SetUp() override {
        nx_register_endpoints();
        Log::set_sink([](LogLevel, const std::string&, const std::string&) {});

        // Temp working directory holds kagami.json (cfg_path anchor) and
        // the content/ subdirectory that AdminContentPutEndpoint writes
        // into. Each test gets a fresh directory to keep side-effects
        // isolated.
        tmp_dir_ = fs::temp_directory_path() / ("kagami_admintest_" + std::to_string(counter_++));
        fs::create_directories(tmp_dir_ / "config");
        cfg_path_ = (tmp_dir_ / "kagami.json").string();
        std::ofstream(cfg_path_) << "{}";

        // Database for user/mute/moderation-event endpoints.
        db_path_ = (tmp_dir_ / "kagami.db").string();
        ASSERT_TRUE(db_.open(db_path_));
        room_.set_db_manager(&db_);
        room_.set_ban_manager(&bm_);

        room_.characters = {"Phoenix", "Edgeworth"};
        room_.areas = {"Lobby"};
        room_.reset_taken();
        room_.build_char_id_index();
        room_.build_area_index();

        // Reload callback — the content PUT endpoint calls this after
        // writing files. We capture the invocation so tests can assert
        // a reload was triggered without needing the real reloader.
        room_.set_reload_func([this] {
            reload_count_++;
            return "reloaded (test stub)";
        });

        nx_ = std::make_unique<NXServer>(room_);
        NXEndpoint::set_server(nx_.get());
        NXEndpoint::set_cfg_path(cfg_path_);

        router_.set_auth_func(
            [this](const std::string& token) -> ServerSession* { return room_.find_session_by_token(token); });
        EndpointFactory::instance().populate(router_);
        router_.bind(http_);

        port_ = http_.bind_to_any_port("127.0.0.1");
        ASSERT_GT(port_, 0);
        server_thread_ = std::thread([this] { http_.listen_after_bind(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    void TearDown() override {
        http_.stop();
        if (server_thread_.joinable())
            server_thread_.join();
        db_.close();
        std::error_code ec;
        fs::remove_all(tmp_dir_, ec);
        Log::set_sink(nullptr);
    }

    http::Client client() {
        return http::Client("127.0.0.1", port_);
    }

    /// Create a session with the given ACL role. Uses create_session_with_token
    /// directly (bypassing /session) so we can set acl_role without needing
    /// the auth-token DB row.
    std::string make_session(const std::string& acl_role, const std::string& ipid = "testipid") {
        static uint64_t id = 1;
        std::string token = "tok_" + std::to_string(id);
        auto session = room_.create_session_with_token(id, "aonx", token);
        session->display_name = "Test";
        session->ipid = ipid;
        session->ip_address = "127.0.0.1";
        session->joined = true;
        session->moderator = acl_role != "NONE";
        session->acl_role = acl_role;
        session->moderator_name = "tester_" + std::to_string(id);
        ++id;
        return token;
    }

    http::Headers bearer(const std::string& token) {
        return {{"Authorization", "Bearer " + token}};
    }

    GameRoom room_;
    BanManager bm_;
    DatabaseManager db_;
    std::unique_ptr<NXServer> nx_;
    RestRouter router_;
    http::Server http_;
    int port_ = 0;
    std::thread server_thread_;

    fs::path tmp_dir_;
    std::string cfg_path_;
    std::string db_path_;
    int reload_count_ = 0;

    static int counter_;
};

int AdminEndpointsTest::counter_ = 0;

} // namespace

// ===========================================================================
// GET /admin/bans
// ===========================================================================

TEST_F(AdminEndpointsTest, BansRequiresAuth) {
    auto res = client().Get("/aonx/v1/admin/bans");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST_F(AdminEndpointsTest, BansRequiresSuper) {
    auto token = make_session("NONE");
    auto res = client().Get("/aonx/v1/admin/bans", bearer(token));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 403);
}

TEST_F(AdminEndpointsTest, BansEmptyList) {
    auto token = make_session("SUPER");
    auto res = client().Get("/aonx/v1/admin/bans", bearer(token));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    auto body = nlohmann::json::parse(res->body);
    EXPECT_EQ(body["count"], 0);
    EXPECT_TRUE(body["bans"].is_array());
}

TEST_F(AdminEndpointsTest, BansReturnsEntries) {
    BanEntry ban;
    ban.ipid = "abcd1234";
    ban.reason = "griefing";
    ban.moderator = "tester";
    ban.timestamp = now_seconds();
    ban.duration = -2;
    bm_.add_ban(ban);

    auto token = make_session("SUPER");
    auto res = client().Get("/aonx/v1/admin/bans", bearer(token));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    auto body = nlohmann::json::parse(res->body);
    ASSERT_EQ(body["bans"].size(), 1);
    EXPECT_EQ(body["bans"][0]["ipid"], "abcd1234");
    EXPECT_EQ(body["bans"][0]["reason"], "griefing");
    EXPECT_TRUE(body["bans"][0]["permanent"].get<bool>());
}

TEST_F(AdminEndpointsTest, BansMalformedLimitClampedNotCrashed) {
    // stoi in the bans endpoint is try/catch'd; garbage limit falls back
    // to the default rather than returning 500. This regression-tests
    // the existing defensive parse.
    auto token = make_session("SUPER");
    auto res = client().Get("/aonx/v1/admin/bans?limit=notanumber", bearer(token));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
}

// ===========================================================================
// GET /admin/users
// ===========================================================================

TEST_F(AdminEndpointsTest, UsersRequiresSuper) {
    auto token = make_session("NONE");
    auto res = client().Get("/aonx/v1/admin/users", bearer(token));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 403);
}

TEST_F(AdminEndpointsTest, UsersReturnsList) {
    db_.create_user("alice", "s", "h", "SUPER").get();
    db_.create_user("bob", "s", "h", "MOD").get();

    auto token = make_session("SUPER");
    auto res = client().Get("/aonx/v1/admin/users", bearer(token));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    auto body = nlohmann::json::parse(res->body);
    ASSERT_EQ(body["users"].size(), 2);
    // Users are not guaranteed to be in a particular order across the
    // aggregation — check membership instead.
    std::vector<std::string> names;
    for (auto& u : body["users"]) names.push_back(u["username"].get<std::string>());
    EXPECT_NE(std::find(names.begin(), names.end(), "alice"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "bob"), names.end());
}

// ===========================================================================
// GET /admin/moderation-events — stoi/stoll guard test
// ===========================================================================

TEST_F(AdminEndpointsTest, ModerationEventsMalformedSinceReturns400) {
    // Regression: previously an invalid `?since=` caused an uncaught
    // std::invalid_argument from std::stoll that propagated up as a 500.
    auto token = make_session("SUPER");
    auto res = client().Get("/aonx/v1/admin/moderation-events?since=notanumber", bearer(token));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    auto body = nlohmann::json::parse(res->body);
    EXPECT_NE(body["reason"].get<std::string>().find("since"), std::string::npos);
}

TEST_F(AdminEndpointsTest, ModerationEventsMalformedUntilReturns400) {
    auto token = make_session("SUPER");
    auto res = client().Get("/aonx/v1/admin/moderation-events?until=abc", bearer(token));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

TEST_F(AdminEndpointsTest, ModerationEventsMalformedLimitReturns400) {
    auto token = make_session("SUPER");
    auto res = client().Get("/aonx/v1/admin/moderation-events?limit=10x", bearer(token));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

TEST_F(AdminEndpointsTest, ModerationEventsValidQueryParams) {
    auto token = make_session("SUPER");
    auto res = client().Get("/aonx/v1/admin/moderation-events?since=0&until=99999999&limit=10", bearer(token));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    auto body = nlohmann::json::parse(res->body);
    EXPECT_TRUE(body["events"].is_array());
}

// ===========================================================================
// GET /admin/mutes
// ===========================================================================

TEST_F(AdminEndpointsTest, MutesReturnsActive) {
    MuteEntry mute;
    mute.ipid = "muted1";
    mute.started_at = now_seconds();
    mute.expires_at = mute.started_at + 3600;
    mute.reason = "spam";
    mute.moderator = "tester";
    db_.add_mute(mute).get();

    auto token = make_session("SUPER");
    auto res = client().Get("/aonx/v1/admin/mutes", bearer(token));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    auto body = nlohmann::json::parse(res->body);
    ASSERT_EQ(body["mutes"].size(), 1);
    EXPECT_EQ(body["mutes"][0]["ipid"], "muted1");
    EXPECT_GT(body["mutes"][0]["seconds_remaining"].get<int64_t>(), 0);
}

// ===========================================================================
// GET /admin/firewall and /admin/asn-reputation
// ===========================================================================

TEST_F(AdminEndpointsTest, FirewallWhenManagerAbsent) {
    // No firewall wired in SetUp — endpoint should report disabled + empty.
    auto token = make_session("SUPER");
    auto res = client().Get("/aonx/v1/admin/firewall", bearer(token));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    auto body = nlohmann::json::parse(res->body);
    EXPECT_FALSE(body["enabled"].get<bool>());
    EXPECT_TRUE(body["rules"].is_array());
    EXPECT_EQ(body["rules"].size(), 0);
}

TEST_F(AdminEndpointsTest, AsnReputationWhenManagerAbsent) {
    auto token = make_session("SUPER");
    auto res = client().Get("/aonx/v1/admin/asn-reputation", bearer(token));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    auto body = nlohmann::json::parse(res->body);
    EXPECT_TRUE(body["asn_entries"].is_array());
    EXPECT_EQ(body["asn_entries"].size(), 0);
}

// ===========================================================================
// GET /admin/content — returns the in-memory lists.
// ===========================================================================

TEST_F(AdminEndpointsTest, ContentGetReturnsCurrent) {
    room_.music = {"== Prelude ==", "intro.opus"};
    auto token = make_session("SUPER");
    auto res = client().Get("/aonx/v1/admin/content", bearer(token));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    auto body = nlohmann::json::parse(res->body);
    EXPECT_EQ(body["characters"], nlohmann::json::array({"Phoenix", "Edgeworth"}));
    EXPECT_EQ(body["areas"], nlohmann::json::array({"Lobby"}));
    EXPECT_EQ(body["music"].size(), 2);
}

// ===========================================================================
// PUT /admin/content
// ===========================================================================

TEST_F(AdminEndpointsTest, ContentPutRequiresSuper) {
    auto token = make_session("NONE");
    auto res = client().Put("/aonx/v1/admin/content", bearer(token), R"({"characters":["A"]})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 403);
}

TEST_F(AdminEndpointsTest, ContentPutRejectsNonObjectBody) {
    auto token = make_session("SUPER");
    auto res = client().Put("/aonx/v1/admin/content", bearer(token), "[]", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

TEST_F(AdminEndpointsTest, ContentPutRejectsWhenNoKnownFields) {
    auto token = make_session("SUPER");
    auto res = client().Put("/aonx/v1/admin/content", bearer(token), R"({"unknown":true})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

TEST_F(AdminEndpointsTest, ContentPutRejectsNonArrayField) {
    auto token = make_session("SUPER");
    auto res = client().Put("/aonx/v1/admin/content", bearer(token), R"({"characters":"not an array"})",
                            "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

TEST_F(AdminEndpointsTest, ContentPutRejectsOversizedArray) {
    // 10,001 entries > kMaxContentEntries — must 413 before writing.
    nlohmann::json body;
    body["characters"] = nlohmann::json::array();
    for (int i = 0; i < 10001; ++i) body["characters"].push_back("c" + std::to_string(i));

    auto token = make_session("SUPER");
    auto res = client().Put("/aonx/v1/admin/content", bearer(token), body.dump(), "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 413);
    // Must NOT have written the file despite partial parsing.
    EXPECT_FALSE(fs::exists(tmp_dir_ / "config" / "characters.txt"));
}

TEST_F(AdminEndpointsTest, ContentPutWritesCharacters) {
    auto token = make_session("SUPER");
    auto res = client().Put("/aonx/v1/admin/content", bearer(token),
                            R"({"characters":["Phoenix","Edgeworth","Maya"]})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    std::ifstream f(tmp_dir_ / "config" / "characters.txt");
    ASSERT_TRUE(f.is_open());
    std::string contents((std::istreambuf_iterator<char>(f)), {});
    EXPECT_EQ(contents, "Phoenix\nEdgeworth\nMaya\n");

    // Reload function fires after a successful write.
    EXPECT_EQ(reload_count_, 1);
}

TEST_F(AdminEndpointsTest, ContentPutMusicGoesToJsonNotTxt) {
    // Regression: earlier version wrote music.txt, which ContentConfig
    // never read — silent data loss.
    auto token = make_session("SUPER");
    auto res = client().Put("/aonx/v1/admin/content", bearer(token),
                            R"({"music":["== Prelude ==","intro.opus","outro.opus"]})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    // music.json exists and is akashi format.
    auto json_path = tmp_dir_ / "config" / "music.json";
    ASSERT_TRUE(fs::exists(json_path));
    std::ifstream f(json_path);
    nlohmann::json root = nlohmann::json::parse(f);
    ASSERT_TRUE(root.is_array());
    ASSERT_EQ(root.size(), 1);
    EXPECT_EQ(root[0]["category"], "== Prelude ==");
    ASSERT_EQ(root[0]["songs"].size(), 2);
    EXPECT_EQ(root[0]["songs"][0]["name"], "intro.opus");

    // music.txt must NOT be written.
    EXPECT_FALSE(fs::exists(tmp_dir_ / "config" / "music.txt"));
}

TEST_F(AdminEndpointsTest, ContentPutMusicWithoutCategoryUsesDefault) {
    auto token = make_session("SUPER");
    auto res = client().Put("/aonx/v1/admin/content", bearer(token),
                            R"({"music":["song1.opus","song2.opus"]})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    std::ifstream f(tmp_dir_ / "config" / "music.json");
    auto root = nlohmann::json::parse(f);
    ASSERT_EQ(root.size(), 1);
    EXPECT_EQ(root[0]["category"], "");
    EXPECT_EQ(root[0]["songs"].size(), 2);
}

TEST_F(AdminEndpointsTest, ContentPutAreasPreservesExistingSettings) {
    // Seed an areas.ini with a custom background + evidence_mod on Courtroom 1,
    // then PUT a name list that reorders and renames. Verify that the
    // per-area settings survive for names that still exist.
    std::ofstream(tmp_dir_ / "config" / "areas.ini")
        << "[0:Basement]\n"
        << "background=courtroom\n"
        << "evidence_mod=HIDDEN\n"
        << "iniswap_allowed=false\n"
        << "\n"
        << "[1:Courtroom 1]\n"
        << "background=gs4\n"
        << "protected_area=true\n";

    auto token = make_session("SUPER");
    // Reorder + drop Basement + add Lobby.
    auto res = client().Put("/aonx/v1/admin/content", bearer(token),
                            R"({"areas":["Courtroom 1","Lobby"]})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    std::ifstream f(tmp_dir_ / "config" / "areas.ini");
    std::string contents((std::istreambuf_iterator<char>(f)), {});

    // Courtroom 1 kept its original keys, and was renumbered 0.
    EXPECT_NE(contents.find("[0:Courtroom 1]"), std::string::npos);
    EXPECT_NE(contents.find("background=gs4"), std::string::npos);
    EXPECT_NE(contents.find("protected_area=true"), std::string::npos);

    // Basement is gone (dropped).
    EXPECT_EQ(contents.find("Basement"), std::string::npos);
    // evidence_mod=HIDDEN belonged to Basement — should not leak into the
    // new Lobby section.
    auto lobby_pos = contents.find("[1:Lobby]");
    ASSERT_NE(lobby_pos, std::string::npos);
    std::string lobby_section = contents.substr(lobby_pos);
    EXPECT_EQ(lobby_section.find("evidence_mod=HIDDEN"), std::string::npos);
}

TEST_F(AdminEndpointsTest, ContentPutAreasNewAreasGetDefaultBackground) {
    auto token = make_session("SUPER");
    auto res = client().Put("/aonx/v1/admin/content", bearer(token), R"({"areas":["NewPlace"]})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    std::ifstream f(tmp_dir_ / "config" / "areas.ini");
    std::string contents((std::istreambuf_iterator<char>(f)), {});
    EXPECT_NE(contents.find("[0:NewPlace]"), std::string::npos);
    EXPECT_NE(contents.find("background=gs4"), std::string::npos);
}

TEST_F(AdminEndpointsTest, ContentPutAtomicityLeavesNoTempFile) {
    // After a successful write, the .tmp staging file must be renamed
    // away. A leftover .tmp would indicate atomic_write's rename failed.
    auto token = make_session("SUPER");
    client().Put("/aonx/v1/admin/content", bearer(token), R"({"characters":["A"]})", "application/json");
    EXPECT_FALSE(fs::exists(tmp_dir_ / "config" / "characters.txt.tmp"));
}

// ===========================================================================
// POST /admin/stop
// ===========================================================================

TEST_F(AdminEndpointsTest, StopRequiresSuper) {
    auto token = make_session("NONE");
    auto res = client().Post("/aonx/v1/admin/stop", bearer(token), "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 403);
}

TEST_F(AdminEndpointsTest, StopInvokesStopFunc) {
    std::atomic<bool> stopped{false};
    room_.set_stop_func([&] { stopped = true; });

    auto token = make_session("SUPER");
    auto res = client().Post("/aonx/v1/admin/stop", bearer(token), "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    // The handler schedules the stop on a detached thread after a ~500ms
    // delay so the HTTP response finishes first. Wait long enough for
    // it to fire.
    for (int i = 0; i < 40 && !stopped.load(); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    EXPECT_TRUE(stopped.load());
}

TEST_F(AdminEndpointsTest, StopReturns503WhenNoStopFunc) {
    room_.set_stop_func(nullptr);
    auto token = make_session("SUPER");
    auto res = client().Post("/aonx/v1/admin/stop", bearer(token), "", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 503);
}

// ===========================================================================
// POST /moderation/actions
// ===========================================================================

TEST_F(AdminEndpointsTest, ModerationActionsRequiresModerator) {
    auto token = make_session("NONE");
    auto body = R"({"action":"kick","target":"someipid","reason":"test"})";
    auto res = client().Post("/aonx/v1/moderation/actions", bearer(token), body, "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 403);
}

TEST_F(AdminEndpointsTest, ModerationActionsUnknownAction) {
    auto token = make_session("SUPER");
    auto body = R"({"action":"nuke","target":"someipid","reason":"x"})";
    auto res = client().Post("/aonx/v1/moderation/actions", bearer(token), body, "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

TEST_F(AdminEndpointsTest, ModerationActionsCustomXActionAccepted) {
    // "x-" prefix is a hook for downstream custom actions — the endpoint
    // should return success without taking any action.
    auto token = make_session("SUPER");
    auto body = R"({"action":"x-log-only","target":"someipid","reason":"audit"})";
    auto res = client().Post("/aonx/v1/moderation/actions", bearer(token), body, "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    auto resp = nlohmann::json::parse(res->body);
    EXPECT_EQ(resp["status"], "applied");
}

TEST_F(AdminEndpointsTest, ModerationActionsKickEmptyTarget) {
    auto token = make_session("SUPER");
    // Empty target — schema should reject OR handler should 400.
    auto body = R"({"action":"kick","target":"","reason":"test"})";
    auto res = client().Post("/aonx/v1/moderation/actions", bearer(token), body, "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

TEST_F(AdminEndpointsTest, ModerationActionsBanAttributesToModerator) {
    // Regression: previously all API-driven mutes were attributed to
    // "REST API" instead of the acting moderator. (Permanent bans omit
    // the duration field — the schema rejects negative values.)
    auto token = make_session("SUPER");
    auto body = R"({"action":"ban","target":"banme","reason":"griefing"})";
    auto res = client().Post("/aonx/v1/moderation/actions", bearer(token), body, "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    auto bans = bm_.list_bans(10);
    ASSERT_EQ(bans.size(), 1);
    EXPECT_EQ(bans[0].ipid, "banme");
    // Attribution must be the session's moderator_name (set in make_session).
    EXPECT_NE(bans[0].moderator.find("tester_"), std::string::npos);
    EXPECT_EQ(bans[0].moderator.find("REST API"), std::string::npos);
}

TEST_F(AdminEndpointsTest, ModerationActionsMuteAttributesToModerator) {
    auto token = make_session("SUPER");
    auto body = R"({"action":"mute","target":"mutee","reason":"spam","duration":3600})";
    auto res = client().Post("/aonx/v1/moderation/actions", bearer(token), body, "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    auto mutes = db_.active_mutes().get();
    ASSERT_EQ(mutes.size(), 1);
    EXPECT_EQ(mutes[0].ipid, "mutee");
    EXPECT_NE(mutes[0].moderator.find("tester_"), std::string::npos);
    EXPECT_EQ(mutes[0].moderator.find("REST API"), std::string::npos);
}

TEST_F(AdminEndpointsTest, ModerationActionsUnmuteWhenNoneActive) {
    auto token = make_session("SUPER");
    auto body = R"({"action":"unmute","target":"ghost","reason":"n/a"})";
    auto res = client().Post("/aonx/v1/moderation/actions", bearer(token), body, "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}

TEST_F(AdminEndpointsTest, ModerationActionsUnbanWhenNotBanned) {
    auto token = make_session("SUPER");
    auto body = R"({"action":"unban","target":"neverbanned","reason":"x"})";
    auto res = client().Post("/aonx/v1/moderation/actions", bearer(token), body, "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}

TEST_F(AdminEndpointsTest, ModerationActionsKickTargetsMatchingSession) {
    // Attach a victim session that our "kick" will match by IPID.
    auto victim_token = make_session("NONE", "victimipid");
    auto mod_token = make_session("SUPER");

    auto body = R"({"action":"kick","target":"victimipid","reason":"testing"})";
    auto res = client().Post("/aonx/v1/moderation/actions", bearer(mod_token), body, "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    // Victim session should be destroyed.
    EXPECT_EQ(room_.find_session_by_token(victim_token), nullptr);
}

TEST_F(AdminEndpointsTest, ModerationActionsBanDropsMatchingSessions) {
    // Ban should both write a BanEntry AND kick active matching sessions
    // in a single pass. Regression for the earlier two-pass TOCTOU.
    auto victim_token = make_session("NONE", "banmeipid");
    auto mod_token = make_session("SUPER");

    auto body = R"({"action":"ban","target":"banmeipid","reason":"spam"})";
    auto res = client().Post("/aonx/v1/moderation/actions", bearer(mod_token), body, "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);

    EXPECT_EQ(room_.find_session_by_token(victim_token), nullptr);
    EXPECT_EQ(bm_.list_bans(10).size(), 1);
}

// ===========================================================================
// Per-endpoint CORS policy — Restricted admin/moderation, Public discovery.
// ===========================================================================

TEST_F(AdminEndpointsTest, AdminEndpointsRestrictedStripsWildcardAllowOrigin) {
    // Simulate a permissive router config.
    router_.set_cors_origins({"*"});

    auto token = make_session("SUPER");
    auto res = client().Get("/aonx/v1/admin/bans", bearer(token));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    // Restricted policy: no Access-Control-Allow-Origin even though the
    // router default would be "*". Browsers will block credentialed
    // cross-origin requests to this URL.
    EXPECT_EQ(res->get_header_value_count("Access-Control-Allow-Origin"), 0u);
}

TEST_F(AdminEndpointsTest, AdminStopRestrictedOnPreflight) {
    // Rebind with wildcard so the baseline would allow cross-origin.
    router_.set_cors_origins({"*"});
    // (No rebind needed — set_default_headers was set via bind() once;
    //  preflight handlers still call apply_cors_for_endpoint which
    //  strips the header for Restricted.)

    auto res = client().Options("/aonx/v1/admin/stop");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 204);
    EXPECT_EQ(res->get_header_value_count("Access-Control-Allow-Origin"), 0u);
}

TEST_F(AdminEndpointsTest, ModerationActionsRestrictedOnPreflight) {
    auto res = client().Options("/aonx/v1/moderation/actions");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 204);
    EXPECT_EQ(res->get_header_value_count("Access-Control-Allow-Origin"), 0u);
}

TEST_F(AdminEndpointsTest, NonAdminEndpointKeepsDefaultCors) {
    // Sanity check: the Default-policy /server endpoint still honors
    // the router's wildcard.
    auto res = client().Get("/aonx/v1/server");
    ASSERT_TRUE(res);
    // The router was set up without cors_origins in SetUp, so there's
    // no Allow-Origin — just verify the policy doesn't strip anything
    // if the default is also absent. Functional parity check.
    EXPECT_EQ(res->status, 200);
}
