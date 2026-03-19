#include "game/ServerList.h"

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string single_server(const std::string& extras = "") {
    return R"([{"name":"Test Server","description":"A test","ip":"127.0.0.1","players":5)" +
           (extras.empty() ? "" : "," + extras) + "}]";
}

// ---------------------------------------------------------------------------
// Parsing
// ---------------------------------------------------------------------------

TEST(ServerList, ParsesSingleEntry) {
    ServerList sl(single_server());
    auto servers = sl.get_servers();
    ASSERT_EQ(servers.size(), 1u);
    EXPECT_EQ(servers[0].name, "Test Server");
    EXPECT_EQ(servers[0].description, "A test");
    EXPECT_EQ(servers[0].hostname, "127.0.0.1");
    EXPECT_EQ(servers[0].players, 5);
}

TEST(ServerList, ParsesMultipleEntries) {
    std::string json = R"([
        {"name":"A","description":"","ip":"1.1.1.1","players":0},
        {"name":"B","description":"","ip":"2.2.2.2","players":10}
    ])";
    ServerList sl(json);
    EXPECT_EQ(sl.get_servers().size(), 2u);
}

TEST(ServerList, EmptyArrayGivesEmptyList) {
    ServerList sl("[]");
    EXPECT_TRUE(sl.get_servers().empty());
}

// ---------------------------------------------------------------------------
// Optional port fields
// ---------------------------------------------------------------------------

TEST(ServerList, WsPortParsed) {
    ServerList sl(single_server(R"("ws_port":8080)"));
    auto s = sl.get_servers()[0];
    ASSERT_TRUE(s.ws_port.has_value());
    EXPECT_EQ(*s.ws_port, 8080u);
    EXPECT_FALSE(s.wss_port.has_value());
    EXPECT_FALSE(s.tcp_port.has_value());
}

TEST(ServerList, WssPortParsed) {
    ServerList sl(single_server(R"("wss_port":443)"));
    auto s = sl.get_servers()[0];
    ASSERT_TRUE(s.wss_port.has_value());
    EXPECT_EQ(*s.wss_port, 443u);
    EXPECT_FALSE(s.ws_port.has_value());
}

TEST(ServerList, TcpPortParsed) {
    ServerList sl(single_server(R"("port":27016)"));
    auto s = sl.get_servers()[0];
    ASSERT_TRUE(s.tcp_port.has_value());
    EXPECT_EQ(*s.tcp_port, 27016u);
}

TEST(ServerList, NoPortFieldsAbsent) {
    ServerList sl(single_server());
    auto s = sl.get_servers()[0];
    EXPECT_FALSE(s.ws_port.has_value());
    EXPECT_FALSE(s.wss_port.has_value());
    EXPECT_FALSE(s.tcp_port.has_value());
}

// ---------------------------------------------------------------------------
// Malformed / missing-key entries are silently skipped
// ---------------------------------------------------------------------------

TEST(ServerList, SkipsEntryMissingRequiredKey) {
    // Missing "players" — entry should be silently dropped.
    std::string json = R"([{"name":"Bad","description":"","ip":"1.2.3.4"}])";
    ServerList sl(json);
    EXPECT_TRUE(sl.get_servers().empty());
}

TEST(ServerList, ValidEntryAfterBadEntryIsKept) {
    std::string json = R"([
        {"name":"Bad","description":"","ip":"1.2.3.4"},
        {"name":"Good","description":"ok","ip":"5.6.7.8","players":1}
    ])";
    ServerList sl(json);
    ASSERT_EQ(sl.get_servers().size(), 1u);
    EXPECT_EQ(sl.get_servers()[0].name, "Good");
}

// ---------------------------------------------------------------------------
// Invalid JSON throws
// ---------------------------------------------------------------------------

TEST(ServerList, InvalidJsonThrows) {
    EXPECT_THROW(ServerList("not json at all"), nlohmann::json::exception);
}
