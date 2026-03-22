#include "event/CharacterListEvent.h"
#include "event/CharsCheckEvent.h"
#include "event/ChatEvent.h"
#include "event/OutgoingICMessageEvent.h"
#include "event/ServerConnectEvent.h"
#include "event/ServerListEvent.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

// ===========================================================================
// CharacterListEvent
// ===========================================================================

TEST(CharacterListEvent, ConstructorStoresValues) {
    CharacterListEvent ev({"Phoenix", "Edgeworth", "Maya"});
    auto& chars = ev.get_characters();
    ASSERT_EQ(chars.size(), 3u);
    EXPECT_EQ(chars[0], "Phoenix");
    EXPECT_EQ(chars[1], "Edgeworth");
    EXPECT_EQ(chars[2], "Maya");
}

TEST(CharacterListEvent, EmptyList) {
    CharacterListEvent ev({});
    EXPECT_TRUE(ev.get_characters().empty());
}

TEST(CharacterListEvent, SingleCharacter) {
    CharacterListEvent ev({"Apollo"});
    ASSERT_EQ(ev.get_characters().size(), 1u);
    EXPECT_EQ(ev.get_characters()[0], "Apollo");
}

TEST(CharacterListEvent, EmptyStringEntries) {
    CharacterListEvent ev({"", "", ""});
    ASSERT_EQ(ev.get_characters().size(), 3u);
    EXPECT_EQ(ev.get_characters()[0], "");
    EXPECT_EQ(ev.get_characters()[1], "");
    EXPECT_EQ(ev.get_characters()[2], "");
}

TEST(CharacterListEvent, ToStringContainsCount) {
    CharacterListEvent ev({"A", "B", "C", "D"});
    std::string s = ev.to_string();
    EXPECT_FALSE(s.empty());
    EXPECT_NE(s.find("4"), std::string::npos);
}

TEST(CharacterListEvent, ToStringEmptyList) {
    CharacterListEvent ev({});
    std::string s = ev.to_string();
    EXPECT_FALSE(s.empty());
    EXPECT_NE(s.find("0"), std::string::npos);
}

// ===========================================================================
// CharsCheckEvent
// ===========================================================================

TEST(CharsCheckEvent, ConstructorStoresValues) {
    CharsCheckEvent ev({true, false, true, false});
    auto& taken = ev.get_taken();
    ASSERT_EQ(taken.size(), 4u);
    EXPECT_TRUE(taken[0]);
    EXPECT_FALSE(taken[1]);
    EXPECT_TRUE(taken[2]);
    EXPECT_FALSE(taken[3]);
}

TEST(CharsCheckEvent, EmptyVector) {
    CharsCheckEvent ev({});
    EXPECT_TRUE(ev.get_taken().empty());
}

TEST(CharsCheckEvent, AllTaken) {
    CharsCheckEvent ev({true, true, true});
    auto& taken = ev.get_taken();
    ASSERT_EQ(taken.size(), 3u);
    for (size_t i = 0; i < taken.size(); ++i) {
        EXPECT_TRUE(taken[i]);
    }
}

TEST(CharsCheckEvent, AllFree) {
    CharsCheckEvent ev({false, false, false});
    auto& taken = ev.get_taken();
    ASSERT_EQ(taken.size(), 3u);
    for (size_t i = 0; i < taken.size(); ++i) {
        EXPECT_FALSE(taken[i]);
    }
}

TEST(CharsCheckEvent, SingleElement) {
    CharsCheckEvent ev({true});
    auto& taken = ev.get_taken();
    ASSERT_EQ(taken.size(), 1u);
    EXPECT_TRUE(taken[0]);
}

// ===========================================================================
// ChatEvent
// ===========================================================================

TEST(ChatEvent, ConstructorStoresValues) {
    ChatEvent ev("Judge", "Order in the court!", false);
    EXPECT_EQ(ev.get_sender_name(), "Judge");
    EXPECT_EQ(ev.get_message(), "Order in the court!");
    EXPECT_FALSE(ev.get_system_message());
}

TEST(ChatEvent, SystemMessage) {
    ChatEvent ev("Server", "Player joined.", true);
    EXPECT_EQ(ev.get_sender_name(), "Server");
    EXPECT_EQ(ev.get_message(), "Player joined.");
    EXPECT_TRUE(ev.get_system_message());
}

TEST(ChatEvent, EmptySenderAndMessage) {
    ChatEvent ev("", "", false);
    EXPECT_EQ(ev.get_sender_name(), "");
    EXPECT_EQ(ev.get_message(), "");
    EXPECT_FALSE(ev.get_system_message());
}

TEST(ChatEvent, EmptySenderName) {
    ChatEvent ev("", "Hello", false);
    EXPECT_EQ(ev.get_sender_name(), "");
    EXPECT_EQ(ev.get_message(), "Hello");
}

TEST(ChatEvent, EmptyMessage) {
    ChatEvent ev("User", "", true);
    EXPECT_EQ(ev.get_sender_name(), "User");
    EXPECT_EQ(ev.get_message(), "");
    EXPECT_TRUE(ev.get_system_message());
}

TEST(ChatEvent, ToStringNonEmpty) {
    ChatEvent ev("Phoenix", "Objection!", false);
    std::string s = ev.to_string();
    EXPECT_FALSE(s.empty());
    EXPECT_NE(s.find("Phoenix"), std::string::npos);
    EXPECT_NE(s.find("Objection!"), std::string::npos);
}

TEST(ChatEvent, ToStringSystemContainsMarker) {
    ChatEvent ev("Server", "Welcome", true);
    std::string s = ev.to_string();
    EXPECT_FALSE(s.empty());
    EXPECT_NE(s.find("SYSTEM"), std::string::npos);
}

TEST(ChatEvent, ToStringNonSystemNoMarker) {
    ChatEvent ev("Player", "Hi", false);
    std::string s = ev.to_string();
    EXPECT_EQ(s.find("SYSTEM"), std::string::npos);
}

// ===========================================================================
// ServerConnectEvent
// ===========================================================================

TEST(ServerConnectEvent, ConstructorStoresValues) {
    ServerConnectEvent ev("127.0.0.1", 27016);
    EXPECT_EQ(ev.get_host(), "127.0.0.1");
    EXPECT_EQ(ev.get_port(), 27016);
}

TEST(ServerConnectEvent, EmptyHost) {
    ServerConnectEvent ev("", 8080);
    EXPECT_EQ(ev.get_host(), "");
    EXPECT_EQ(ev.get_port(), 8080);
}

TEST(ServerConnectEvent, ZeroPort) {
    ServerConnectEvent ev("localhost", 0);
    EXPECT_EQ(ev.get_host(), "localhost");
    EXPECT_EQ(ev.get_port(), 0);
}

TEST(ServerConnectEvent, MaxPort) {
    ServerConnectEvent ev("example.com", 65535);
    EXPECT_EQ(ev.get_host(), "example.com");
    EXPECT_EQ(ev.get_port(), 65535);
}

TEST(ServerConnectEvent, HostnameWithPort) {
    ServerConnectEvent ev("play.aceattorneyonline.com", 27016);
    EXPECT_EQ(ev.get_host(), "play.aceattorneyonline.com");
    EXPECT_EQ(ev.get_port(), 27016);
}

TEST(ServerConnectEvent, IPv6Host) {
    ServerConnectEvent ev("::1", 9000);
    EXPECT_EQ(ev.get_host(), "::1");
    EXPECT_EQ(ev.get_port(), 9000);
}

// ===========================================================================
// ServerListEvent
// ===========================================================================

// Helper: build a JSON string with one server that has a ws_port.
static std::string make_server_json(const std::string& name, const std::string& ip,
                                    int players, int ws_port) {
    return R"([{"name":")" + name + R"(","description":"desc","ip":")" + ip +
           R"(","players":)" + std::to_string(players) +
           R"(,"ws_port":)" + std::to_string(ws_port) + "}]";
}

TEST(ServerListEvent, ConstructorAndGetter) {
    ServerList sl(make_server_json("TestServer", "10.0.0.1", 5, 8080));
    ServerListEvent ev(sl);
    auto result = ev.get_server_list();
    auto servers = result.get_servers();
    ASSERT_EQ(servers.size(), 1u);
    EXPECT_EQ(servers[0].name, "TestServer");
    EXPECT_EQ(servers[0].hostname, "10.0.0.1");
    EXPECT_EQ(servers[0].players, 5);
    ASSERT_TRUE(servers[0].ws_port.has_value());
    EXPECT_EQ(*servers[0].ws_port, 8080);
}

TEST(ServerListEvent, EmptyServerList) {
    ServerList sl("[]");
    ServerListEvent ev(sl);
    auto result = ev.get_server_list();
    EXPECT_TRUE(result.get_servers().empty());
}

TEST(ServerListEvent, ToStringNonEmpty) {
    ServerList sl(make_server_json("MyServer", "1.2.3.4", 10, 9090));
    ServerListEvent ev(sl);
    std::string s = ev.to_string();
    EXPECT_FALSE(s.empty());
    EXPECT_NE(s.find("1.2.3.4"), std::string::npos);
    EXPECT_NE(s.find("9090"), std::string::npos);
}

TEST(ServerListEvent, ToStringEmptyList) {
    ServerList sl("[]");
    ServerListEvent ev(sl);
    std::string s = ev.to_string();
    // Empty server list produces an empty string (no servers to format).
    EXPECT_TRUE(s.empty());
}

TEST(ServerListEvent, MultipleServers) {
    std::string json = R"([
        {"name":"A","description":"","ip":"1.1.1.1","players":0,"ws_port":8001},
        {"name":"B","description":"","ip":"2.2.2.2","players":3,"ws_port":8002}
    ])";
    ServerList sl(json);
    ServerListEvent ev(sl);
    auto result = ev.get_server_list();
    auto servers = result.get_servers();
    ASSERT_EQ(servers.size(), 2u);
    EXPECT_EQ(servers[0].name, "A");
    EXPECT_EQ(servers[1].name, "B");
}

// ===========================================================================
// OutgoingICMessageEvent
// ===========================================================================

TEST(OutgoingICMessageEvent, ConstructorStoresValues) {
    ICMessageData msg;
    msg.character = "Phoenix";
    msg.emote = "normal";
    msg.message = "Take that!";
    msg.side = "def";
    msg.char_id = 5;
    msg.text_color = 2;

    OutgoingICMessageEvent ev(msg);
    EXPECT_EQ(ev.data().character, "Phoenix");
    EXPECT_EQ(ev.data().emote, "normal");
    EXPECT_EQ(ev.data().message, "Take that!");
    EXPECT_EQ(ev.data().side, "def");
    EXPECT_EQ(ev.data().char_id, 5);
    EXPECT_EQ(ev.data().text_color, 2);
}

TEST(OutgoingICMessageEvent, DefaultValues) {
    ICMessageData msg;
    OutgoingICMessageEvent ev(msg);
    EXPECT_EQ(ev.data().desk_mod, 1);
    EXPECT_EQ(ev.data().pre_emote, "");
    EXPECT_EQ(ev.data().character, "");
    EXPECT_EQ(ev.data().emote, "");
    EXPECT_EQ(ev.data().message, "");
    EXPECT_EQ(ev.data().side, "def");
    EXPECT_EQ(ev.data().sfx_name, "");
    EXPECT_EQ(ev.data().emote_mod, 0);
    EXPECT_EQ(ev.data().char_id, -1);
    EXPECT_EQ(ev.data().sfx_delay, 0);
    EXPECT_EQ(ev.data().objection_mod, 0);
    EXPECT_EQ(ev.data().evidence_id, 0);
    EXPECT_EQ(ev.data().flip, 0);
    EXPECT_EQ(ev.data().realization, 0);
    EXPECT_EQ(ev.data().text_color, 0);
    EXPECT_EQ(ev.data().showname, "");
    EXPECT_EQ(ev.data().other_charid, -1);
    EXPECT_EQ(ev.data().other_name, "");
    EXPECT_EQ(ev.data().other_emote, "");
    EXPECT_EQ(ev.data().self_offset, "0&0");
    EXPECT_EQ(ev.data().other_offset, "0&0");
    EXPECT_EQ(ev.data().other_flip, 0);
    EXPECT_EQ(ev.data().immediate, 0);
    EXPECT_EQ(ev.data().looping_sfx, 0);
    EXPECT_EQ(ev.data().screenshake, 0);
    EXPECT_EQ(ev.data().frame_screenshake, "");
    EXPECT_EQ(ev.data().frame_realization, "");
    EXPECT_EQ(ev.data().frame_sfx, "");
    EXPECT_EQ(ev.data().additive, 0);
    EXPECT_EQ(ev.data().effects, "");
    EXPECT_EQ(ev.data().blipname, "");
    EXPECT_EQ(ev.data().slide, "");
}

TEST(OutgoingICMessageEvent, EmptyStrings) {
    ICMessageData msg;
    msg.character = "";
    msg.emote = "";
    msg.message = "";
    msg.side = "";
    msg.showname = "";
    OutgoingICMessageEvent ev(msg);
    EXPECT_EQ(ev.data().character, "");
    EXPECT_EQ(ev.data().emote, "");
    EXPECT_EQ(ev.data().message, "");
    EXPECT_EQ(ev.data().side, "");
    EXPECT_EQ(ev.data().showname, "");
}

TEST(OutgoingICMessageEvent, AllFieldsPopulated) {
    ICMessageData msg;
    msg.desk_mod = 0;
    msg.pre_emote = "deskslam";
    msg.character = "Edgeworth";
    msg.emote = "confident";
    msg.message = "Objection!";
    msg.side = "pro";
    msg.sfx_name = "objection";
    msg.emote_mod = 6;
    msg.char_id = 12;
    msg.sfx_delay = 500;
    msg.objection_mod = 1;
    msg.evidence_id = 3;
    msg.flip = 1;
    msg.realization = 1;
    msg.text_color = 4;
    msg.showname = "Miles Edgeworth";
    msg.other_charid = 7;
    msg.other_name = "Phoenix";
    msg.other_emote = "sweating";
    msg.self_offset = "10&-5";
    msg.other_offset = "-10&5";
    msg.other_flip = 1;
    msg.immediate = 1;
    msg.looping_sfx = 1;
    msg.screenshake = 1;
    msg.frame_screenshake = "1=shake";
    msg.frame_realization = "2=flash";
    msg.frame_sfx = "3=sfx";
    msg.additive = 1;
    msg.effects = "fade_in";
    msg.blipname = "male";
    msg.slide = "right";

    OutgoingICMessageEvent ev(msg);
    EXPECT_EQ(ev.data().desk_mod, 0);
    EXPECT_EQ(ev.data().pre_emote, "deskslam");
    EXPECT_EQ(ev.data().character, "Edgeworth");
    EXPECT_EQ(ev.data().emote, "confident");
    EXPECT_EQ(ev.data().message, "Objection!");
    EXPECT_EQ(ev.data().side, "pro");
    EXPECT_EQ(ev.data().sfx_name, "objection");
    EXPECT_EQ(ev.data().emote_mod, 6);
    EXPECT_EQ(ev.data().char_id, 12);
    EXPECT_EQ(ev.data().sfx_delay, 500);
    EXPECT_EQ(ev.data().objection_mod, 1);
    EXPECT_EQ(ev.data().evidence_id, 3);
    EXPECT_EQ(ev.data().flip, 1);
    EXPECT_EQ(ev.data().realization, 1);
    EXPECT_EQ(ev.data().text_color, 4);
    EXPECT_EQ(ev.data().showname, "Miles Edgeworth");
    EXPECT_EQ(ev.data().other_charid, 7);
    EXPECT_EQ(ev.data().other_name, "Phoenix");
    EXPECT_EQ(ev.data().other_emote, "sweating");
    EXPECT_EQ(ev.data().self_offset, "10&-5");
    EXPECT_EQ(ev.data().other_offset, "-10&5");
    EXPECT_EQ(ev.data().other_flip, 1);
    EXPECT_EQ(ev.data().immediate, 1);
    EXPECT_EQ(ev.data().looping_sfx, 1);
    EXPECT_EQ(ev.data().screenshake, 1);
    EXPECT_EQ(ev.data().frame_screenshake, "1=shake");
    EXPECT_EQ(ev.data().frame_realization, "2=flash");
    EXPECT_EQ(ev.data().frame_sfx, "3=sfx");
    EXPECT_EQ(ev.data().additive, 1);
    EXPECT_EQ(ev.data().effects, "fade_in");
    EXPECT_EQ(ev.data().blipname, "male");
    EXPECT_EQ(ev.data().slide, "right");
}

TEST(OutgoingICMessageEvent, ToStringContainsFields) {
    ICMessageData msg;
    msg.character = "Godot";
    msg.emote = "coffee";
    msg.message = "That's my coffee.";
    OutgoingICMessageEvent ev(msg);
    std::string s = ev.to_string();
    EXPECT_FALSE(s.empty());
    EXPECT_NE(s.find("Godot"), std::string::npos);
    EXPECT_NE(s.find("coffee"), std::string::npos);
    EXPECT_NE(s.find("That's my coffee."), std::string::npos);
}

TEST(OutgoingICMessageEvent, ToStringWithEmptyData) {
    ICMessageData msg;
    OutgoingICMessageEvent ev(msg);
    std::string s = ev.to_string();
    EXPECT_FALSE(s.empty());
}
