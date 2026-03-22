#include "net/ao/AOClient.h"
#include "net/ao/AOPacket.h"
#include "net/ao/PacketTypes.h"

#include "ao/event/ICMessageEvent.h"
#include "ao/event/PlayerCountEvent.h"
#include "ao/event/ServerInfoEvent.h"
#include "event/AreaUpdateEvent.h"
#include "event/AssetUrlEvent.h"
#include "event/BackgroundEvent.h"
#include "event/CharacterListEvent.h"
#include "event/CharsCheckEvent.h"
#include "event/ChatEvent.h"
#include "event/EventManager.h"
#include "event/EvidenceListEvent.h"
#include "event/FeatureListEvent.h"
#include "event/HealthBarEvent.h"
#include "event/MusicChangeEvent.h"
#include "event/MusicListEvent.h"
#include "event/PlayerListEvent.h"
#include "event/TimerEvent.h"
#include "event/UIEvent.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helper: drain an event channel to prevent singleton state leaking.
// ---------------------------------------------------------------------------
template <typename T>
static void drain(EventChannel<T>& ch) {
    while (ch.get_event()) {
    }
}

/// Drain all event channels that AOClient / packet handlers may write to.
static void drain_all_channels() {
    auto& em = EventManager::instance();
    drain(em.get_channel<BackgroundEvent>());
    drain(em.get_channel<ChatEvent>());
    drain(em.get_channel<UIEvent>());
    drain(em.get_channel<CharacterListEvent>());
    drain(em.get_channel<CharsCheckEvent>());
    drain(em.get_channel<ICMessageEvent>());
    drain(em.get_channel<ServerInfoEvent>());
    drain(em.get_channel<PlayerCountEvent>());
    drain(em.get_channel<AssetUrlEvent>());
    drain(em.get_channel<FeatureListEvent>());
    drain(em.get_channel<MusicListEvent>());
    drain(em.get_channel<MusicChangeEvent>());
    drain(em.get_channel<AreaUpdateEvent>());
    drain(em.get_channel<HealthBarEvent>());
    drain(em.get_channel<TimerEvent>());
    drain(em.get_channel<EvidenceListEvent>());
    drain(em.get_channel<PlayerListEvent>());
}

// ---------------------------------------------------------------------------
// Fixture: creates a fresh AOClient and drains channels on teardown.
// ---------------------------------------------------------------------------
class AOClientTest : public ::testing::Test {
  protected:
    void SetUp() override {
        drain_all_channels();
        client = std::make_unique<AOClient>();
    }

    void TearDown() override {
        client.reset();
        drain_all_channels();
    }

    std::unique_ptr<AOClient> client;
};

// ===========================================================================
// 1. Initial state
// ===========================================================================

TEST_F(AOClientTest, InitialStateIsNotConnected) {
    EXPECT_EQ(client->conn_state, NOT_CONNECTED);
}

TEST_F(AOClientTest, InitialCharIdIsNegativeOne) {
    EXPECT_EQ(client->char_id, -1);
}

TEST_F(AOClientTest, InitialPlayerNumberIsZero) {
    EXPECT_EQ(client->player_number, 0);
}

TEST_F(AOClientTest, InitialCharacterListIsEmpty) {
    EXPECT_TRUE(client->character_list.empty());
}

TEST_F(AOClientTest, InitialFeaturesIsEmpty) {
    EXPECT_TRUE(client->features.empty());
}

TEST_F(AOClientTest, FlushOutgoingIsEmptyInitially) {
    // Before any connection, nothing should be queued.
    auto msgs = client->flush_outgoing();
    EXPECT_TRUE(msgs.empty());
}

// ===========================================================================
// 2. on_connect() transitions state
// ===========================================================================

TEST_F(AOClientTest, OnConnectTransitionsToConnected) {
    client->on_connect();
    EXPECT_EQ(client->conn_state, CONNECTED);
}

TEST_F(AOClientTest, OnConnectDoesNotQueuePackets) {
    // on_connect itself does not queue handshake packets; those are triggered
    // by server-sent packets (decryptor, etc.).
    client->on_connect();
    auto msgs = client->flush_outgoing();
    EXPECT_TRUE(msgs.empty());
}

// ===========================================================================
// 3. on_message() with valid packets publishes correct events
// ===========================================================================

TEST_F(AOClientTest, OnMessageDecryptorQueuesHI) {
    client->on_connect();

    // Server sends decryptor packet.
    client->on_message("decryptor#34#%");

    auto msgs = client->flush_outgoing();
    // Decryptor handler queues an HI packet.
    ASSERT_GE(msgs.size(), 1u);
    // HI packet starts with "HI#"
    EXPECT_EQ(msgs[0].substr(0, 3), "HI#");
}

TEST_F(AOClientTest, OnMessageIDSetsPlayerNumberAndPublishesEvent) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<ServerInfoEvent>();

    // Server sends ID packet: player_number#software#version
    client->on_message("ID#42#TestServer#1.0.0#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_software(), "TestServer");
    EXPECT_EQ(ev->get_version(), "1.0.0");
    EXPECT_EQ(ev->get_player_num(), 42);

    EXPECT_EQ(client->player_number, 42);

    // ID handler queues an ID response (client identification).
    auto msgs = client->flush_outgoing();
    ASSERT_GE(msgs.size(), 1u);
    EXPECT_EQ(msgs[0].substr(0, 3), "ID#");
}

TEST_F(AOClientTest, OnMessagePNPublishesPlayerCountAndQueuesAskChaa) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<PlayerCountEvent>();

    // PN: current_players#max_players#description
    client->on_message("PN#5#10#Welcome to the server#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_current(), 5);
    EXPECT_EQ(ev->get_max(), 10);

    // PN handler queues an askchaa packet.
    auto msgs = client->flush_outgoing();
    ASSERT_GE(msgs.size(), 1u);
    // askchaa serializes as "askchaa#%"
    EXPECT_EQ(msgs[0], "askchaa#%");
}

TEST_F(AOClientTest, OnMessageSCPublishesCharacterListAndQueuesRM) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<CharacterListEvent>();

    // SC: character entries separated by '#', each entry is "folder&display"
    client->on_message("SC#Phoenix&Phoenix Wright#Edgeworth&Miles Edgeworth#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    const auto& names = ev->get_characters();
    ASSERT_EQ(names.size(), 2u);
    EXPECT_EQ(names[0], "Phoenix");
    EXPECT_EQ(names[1], "Edgeworth");

    // SC also populates the client character list.
    ASSERT_EQ(client->character_list.size(), 2u);
    EXPECT_EQ(client->character_list[0], "Phoenix&Phoenix Wright");

    // SC handler queues an RM (request music) packet.
    auto msgs = client->flush_outgoing();
    ASSERT_GE(msgs.size(), 1u);
    EXPECT_EQ(msgs[0], "RM#%");
}

TEST_F(AOClientTest, OnMessageBNPublishesBackgroundEvent) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<BackgroundEvent>();

    // BN: background#position
    client->on_message("BN#gs4#def#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_background(), "gs4");
    EXPECT_EQ(ev->get_position(), "def");
}

TEST_F(AOClientTest, OnMessageCTPublishesChatEvent) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<ChatEvent>();

    // CT: sender#message#system_flag
    client->on_message("CT#Phoenix#Objection!#0#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_sender_name(), "Phoenix");
    EXPECT_EQ(ev->get_message(), "Objection!");
    EXPECT_FALSE(ev->get_system_message());
}

TEST_F(AOClientTest, OnMessageFLSetsFeatures) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<FeatureListEvent>();

    client->on_message("FL#yellowtext#flipping#customobjections#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    const auto& feats = ev->features();
    ASSERT_EQ(feats.size(), 3u);
    EXPECT_EQ(feats[0], "yellowtext");
    EXPECT_EQ(feats[1], "flipping");
    EXPECT_EQ(feats[2], "customobjections");

    // Also sets client features.
    ASSERT_EQ(client->features.size(), 3u);
    EXPECT_EQ(client->features[0], "yellowtext");
}

TEST_F(AOClientTest, OnMessageDONETransitionsToJoined) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<UIEvent>();

    client->on_message("DONE#%");

    EXPECT_EQ(client->conn_state, JOINED);

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_type(), CHAR_LOADING_DONE);
}

TEST_F(AOClientTest, OnMessagePVSetsCharIdAndPublishesEvent) {
    client->on_connect();
    // Provide a character list so PV can resolve the character name.
    client->character_list = {"Phoenix&Phoenix Wright", "Edgeworth&Miles Edgeworth"};

    auto& ch = EventManager::instance().get_channel<UIEvent>();

    // PV: player_number#CID#char_id
    client->on_message("PV#0#CID#1#%");

    EXPECT_EQ(client->char_id, 1);

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_type(), ENTERED_COURTROOM);
    EXPECT_EQ(ev->get_char_id(), 1);
}

TEST_F(AOClientTest, OnMessageMCPublishesMusicChangeEvent) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<MusicChangeEvent>();

    // MC: name#char_id#showname#looping#channel#effects
    client->on_message("MC#Trial.opus#0#Phoenix#1#0#0#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->track(), "Trial.opus");
    EXPECT_EQ(ev->char_id(), 0);
    EXPECT_EQ(ev->showname(), "Phoenix");
}

TEST_F(AOClientTest, OnMessageHPPublishesHealthBarEvent) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<HealthBarEvent>();

    // HP: side#value
    client->on_message("HP#1#7#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->side(), 1);
    EXPECT_EQ(ev->value(), 7);
}

TEST_F(AOClientTest, OnMessageTIPublishesTimerEvent) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<TimerEvent>();

    // TI: timer_id#action#time_ms
    client->on_message("TI#0#0#30000#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->timer_id(), 0);
    EXPECT_EQ(ev->action(), 0);
    EXPECT_EQ(ev->time_ms(), 30000);
}

TEST_F(AOClientTest, OnMessageLEPublishesEvidenceListEvent) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<EvidenceListEvent>();

    // LE: each field is "name&description&image"
    client->on_message("LE#Badge&Attorney's badge&badge.png#Autopsy&Autopsy report&autopsy.png#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    const auto& items = ev->items();
    ASSERT_EQ(items.size(), 2u);
    EXPECT_EQ(items[0].name, "Badge");
    EXPECT_EQ(items[0].description, "Attorney's badge");
    EXPECT_EQ(items[0].image, "badge.png");
    EXPECT_EQ(items[1].name, "Autopsy");
}

TEST_F(AOClientTest, OnMessageARUPPublishesAreaUpdateEvent) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<AreaUpdateEvent>();

    // ARUP: type#value1#value2#...
    client->on_message("ARUP#0#3#5#2#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->type(), AreaUpdateEvent::PLAYERS);
    const auto& vals = ev->values();
    ASSERT_EQ(vals.size(), 3u);
    EXPECT_EQ(vals[0], "3");
    EXPECT_EQ(vals[1], "5");
    EXPECT_EQ(vals[2], "2");
}

TEST_F(AOClientTest, OnMessagePRPublishesPlayerListAddEvent) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<PlayerListEvent>();

    // PR: player_id#update_type (0=add, 1=remove)
    client->on_message("PR#42#0#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->action(), PlayerListEvent::Action::ADD);
    EXPECT_EQ(ev->player_id(), 42);
}

TEST_F(AOClientTest, OnMessagePUPublishesPlayerListUpdateEvent) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<PlayerListEvent>();

    // PU: player_id#data_type#data
    client->on_message("PU#7#0#SomeName#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->action(), PlayerListEvent::Action::UPDATE_NAME);
    EXPECT_EQ(ev->player_id(), 7);
    EXPECT_EQ(ev->data(), "SomeName");
}

TEST_F(AOClientTest, OnMessageASSWithUrlPublishesAssetUrlEvent) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<AssetUrlEvent>();

    client->on_message("ASS#https://example.com/assets/#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->url(), "https://example.com/assets/");

    EXPECT_EQ(client->asset_url, "https://example.com/assets/");
}

TEST_F(AOClientTest, OnMessageASSWithEmptyUrlDoesNotPublish) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<AssetUrlEvent>();

    client->on_message("ASS##%");

    // Empty URL should not publish an AssetUrlEvent.
    auto ev = ch.get_event();
    EXPECT_FALSE(ev.has_value());
}

TEST_F(AOClientTest, OnMessageSISetsCountsAndQueuesRC) {
    client->on_connect();

    // SI: character_count#evidence_count#music_count
    client->on_message("SI#80#12#50#%");

    EXPECT_EQ(client->character_count, 80);
    EXPECT_EQ(client->evidence_count, 12);
    EXPECT_EQ(client->music_count, 50);

    // SI handler queues an RC (request characters) packet.
    auto msgs = client->flush_outgoing();
    ASSERT_GE(msgs.size(), 1u);
    EXPECT_EQ(msgs[0], "RC#%");
}

TEST_F(AOClientTest, OnMessageSMPublishesMusicListAndQueuesRD) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<MusicListEvent>();

    // SM contains areas then music. First audio-extension entry marks the boundary.
    client->on_message("SM#Lobby#Courtroom#Trial.opus#Phoenix.mp3#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    // "Lobby" is an area. "Courtroom" preceded the first audio track so becomes a category header.
    // The areas list should be ["Lobby"], tracks should contain ["Courtroom", "Trial.opus", "Phoenix.mp3"]
    EXPECT_FALSE(ev->areas().empty());
    EXPECT_FALSE(ev->tracks().empty());

    // SM handler queues an RD (ready/done) packet.
    auto msgs = client->flush_outgoing();
    ASSERT_GE(msgs.size(), 1u);
    EXPECT_EQ(msgs[0], "RD#%");
}

// ===========================================================================
// 4. on_message() with malformed data doesn't crash
// ===========================================================================

TEST_F(AOClientTest, MalformedPacketNoCrash) {
    client->on_connect();

    // No delimiter — should be buffered but not crash.
    EXPECT_NO_THROW(client->on_message("garbage data no delimiter"));
    auto msgs = client->flush_outgoing();
    EXPECT_TRUE(msgs.empty());
}

TEST_F(AOClientTest, EmptyMessageNoCrash) {
    client->on_connect();
    EXPECT_NO_THROW(client->on_message(""));
}

TEST_F(AOClientTest, DelimiterOnlyNoCrash) {
    client->on_connect();
    // Just a delimiter with no header — should not crash.
    EXPECT_NO_THROW(client->on_message("#%"));
}

TEST_F(AOClientTest, UnknownPacketTypeNoCrash) {
    client->on_connect();
    EXPECT_NO_THROW(client->on_message("ZZUNKNOWN#field1#field2#%"));
}

TEST_F(AOClientTest, PacketWithWrongStateDoesNotCrash) {
    // Client is NOT_CONNECTED; sending a DONE packet (which requires CONNECTED)
    // should throw internally but be caught by on_message's try/catch.
    EXPECT_NO_THROW(client->on_message("DONE#%"));
    // State should remain NOT_CONNECTED since the exception was caught.
    EXPECT_EQ(client->conn_state, NOT_CONNECTED);
}

TEST_F(AOClientTest, TruncatedIDPacketNoCrash) {
    client->on_connect();
    // ID needs 3 fields; provide only 1 — should not crash (caught as invalid or exception).
    EXPECT_NO_THROW(client->on_message("ID#42#%"));
}

TEST_F(AOClientTest, MultiplePacketsInOneMessage) {
    client->on_connect();

    auto& bg_ch = EventManager::instance().get_channel<BackgroundEvent>();
    auto& chat_ch = EventManager::instance().get_channel<ChatEvent>();

    // Two complete packets concatenated in one TCP receive.
    client->on_message("BN#default#wit#%CT#Server#Hello#1#%");

    auto bg_ev = bg_ch.get_event();
    ASSERT_TRUE(bg_ev.has_value());
    EXPECT_EQ(bg_ev->get_background(), "default");

    auto chat_ev = chat_ch.get_event();
    ASSERT_TRUE(chat_ev.has_value());
    EXPECT_EQ(chat_ev->get_sender_name(), "Server");
}

TEST_F(AOClientTest, SplitPacketAcrossMultipleMessages) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<BackgroundEvent>();

    // First half of the packet.
    client->on_message("BN#court");
    auto ev = ch.get_event();
    EXPECT_FALSE(ev.has_value()); // Not complete yet.

    // Second half arrives later.
    client->on_message("room#def#%");
    ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_background(), "courtroom");
    EXPECT_EQ(ev->get_position(), "def");
}

// ===========================================================================
// 5. on_disconnect() resets state
// ===========================================================================

TEST_F(AOClientTest, OnDisconnectResetsToNotConnected) {
    client->on_connect();
    EXPECT_EQ(client->conn_state, CONNECTED);

    client->on_disconnect();
    EXPECT_EQ(client->conn_state, NOT_CONNECTED);
}

TEST_F(AOClientTest, OnDisconnectFromJoinedResetsToNotConnected) {
    client->on_connect();
    client->on_message("DONE#%"); // Transition to JOINED
    EXPECT_EQ(client->conn_state, JOINED);

    client->on_disconnect();
    EXPECT_EQ(client->conn_state, NOT_CONNECTED);
}

// ===========================================================================
// 6. flush_outgoing() returns and clears queued packets
// ===========================================================================

TEST_F(AOClientTest, FlushOutgoingReturnsQueuedPackets) {
    client->on_connect();

    // Trigger a packet handler that queues a response.
    client->on_message("decryptor#34#%");

    auto msgs = client->flush_outgoing();
    EXPECT_FALSE(msgs.empty());
}

TEST_F(AOClientTest, FlushOutgoingClearsQueueAfterReturn) {
    client->on_connect();

    client->on_message("decryptor#34#%");

    auto msgs1 = client->flush_outgoing();
    EXPECT_FALSE(msgs1.empty());

    // Second flush should return empty since queue was cleared.
    auto msgs2 = client->flush_outgoing();
    EXPECT_TRUE(msgs2.empty());
}

TEST_F(AOClientTest, AddMessageQueuesSerializedPacket) {
    AOPacketHI hi("testhwid");
    client->add_message(hi);

    auto msgs = client->flush_outgoing();
    ASSERT_EQ(msgs.size(), 1u);
    EXPECT_EQ(msgs[0], "HI#testhwid#%");
}

TEST_F(AOClientTest, MultipleAddMessagesQueueInOrder) {
    AOPacketHI hi("hwid");
    AOPacketPW pw("pass");

    client->add_message(hi);
    client->add_message(pw);

    auto msgs = client->flush_outgoing();
    ASSERT_EQ(msgs.size(), 2u);
    EXPECT_EQ(msgs[0], "HI#hwid#%");
    EXPECT_EQ(msgs[1], "PW#pass#%");
}

// ===========================================================================
// 7. Packet dispatch for known types — full handshake flow
// ===========================================================================

TEST_F(AOClientTest, FullHandshakeSequenceFlow) {
    // Simulate the entire server handshake sequence and verify state transitions.

    // Step 1: Connect
    client->on_connect();
    EXPECT_EQ(client->conn_state, CONNECTED);

    // Step 2: Server sends decryptor -> client responds with HI
    client->on_message("decryptor#34#%");
    auto msgs = client->flush_outgoing();
    ASSERT_GE(msgs.size(), 1u);
    EXPECT_EQ(msgs[0].substr(0, 3), "HI#");

    // Step 3: Server sends ID -> client responds with ID
    client->on_message("ID#1#tsuserver3#3.0.0#%");
    EXPECT_EQ(client->player_number, 1);
    msgs = client->flush_outgoing();
    ASSERT_GE(msgs.size(), 1u);
    EXPECT_EQ(msgs[0].substr(0, 3), "ID#");

    // Step 4: Server sends FL (feature list)
    client->on_message("FL#yellowtext#flipping#%");
    ASSERT_EQ(client->features.size(), 2u);

    // Step 5: Server sends ASS (asset URL)
    client->on_message("ASS#https://example.com/#%");
    EXPECT_EQ(client->asset_url, "https://example.com/");

    // Step 6: Server sends PN (player count) -> client sends askchaa
    client->on_message("PN#3#16#%");
    msgs = client->flush_outgoing();
    ASSERT_GE(msgs.size(), 1u);
    EXPECT_EQ(msgs[0], "askchaa#%");

    // Step 7: Server sends SI (server info) -> client sends RC
    client->on_message("SI#2#0#3#%");
    EXPECT_EQ(client->character_count, 2);
    msgs = client->flush_outgoing();
    ASSERT_GE(msgs.size(), 1u);
    EXPECT_EQ(msgs[0], "RC#%");

    // Step 8: Server sends SC (character list) -> client sends RM
    client->on_message("SC#Phoenix&Phoenix Wright#Edgeworth&Miles Edgeworth#%");
    EXPECT_EQ(client->character_list.size(), 2u);
    msgs = client->flush_outgoing();
    ASSERT_GE(msgs.size(), 1u);
    EXPECT_EQ(msgs[0], "RM#%");

    // Step 9: Server sends SM (music list) -> client sends RD
    client->on_message("SM#Lobby#Trial.opus#%");
    msgs = client->flush_outgoing();
    ASSERT_GE(msgs.size(), 1u);
    EXPECT_EQ(msgs[0], "RD#%");

    // Step 10: Server sends DONE -> state transitions to JOINED
    client->on_message("DONE#%");
    EXPECT_EQ(client->conn_state, JOINED);
}

TEST_F(AOClientTest, CHECKPacketDoesNotCrash) {
    client->on_connect();
    // Server CHECK is a keepalive response; handler is a no-op.
    EXPECT_NO_THROW(client->on_message("CHECK#%"));
}

TEST_F(AOClientTest, CharsCheckPublishesEvent) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<CharsCheckEvent>();

    client->on_message("CharsCheck#-1#0#-1#0#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    const auto& taken = ev->get_taken();
    ASSERT_EQ(taken.size(), 4u);
    EXPECT_TRUE(taken[0]);
    EXPECT_FALSE(taken[1]);
    EXPECT_TRUE(taken[2]);
    EXPECT_FALSE(taken[3]);
}

TEST_F(AOClientTest, MSPacketPublishesICMessageEvent) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<ICMessageEvent>();

    // MS packet with the minimum 15 fields:
    // desk_mod#pre_emote#character#emote#message#side#sfx_name#emote_mod#
    // char_id#sfx_delay#shout_mod#evidence#flip#realization#text_color
    client->on_message("MS#1#happy#Phoenix#normal#Hello world!#def#sfx.wav#1#0#0#0#0#0#0#0#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_character(), "Phoenix");
    EXPECT_EQ(ev->get_message(), "Hello world!");
    EXPECT_EQ(ev->get_side(), "def");
}

TEST_F(AOClientTest, FAPackishesPartialMusicListEvent) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<MusicListEvent>();

    client->on_message("FA#Lobby#Basement#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_TRUE(ev->partial());
    ASSERT_EQ(ev->areas().size(), 2u);
    EXPECT_EQ(ev->areas()[0], "Lobby");
    EXPECT_TRUE(ev->tracks().empty());
}

TEST_F(AOClientTest, FMPublishesPartialMusicListEvent) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<MusicListEvent>();

    client->on_message("FM#Trial.mp3#Phoenix.ogg#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_TRUE(ev->partial());
    EXPECT_TRUE(ev->areas().empty());
    ASSERT_EQ(ev->tracks().size(), 2u);
    EXPECT_EQ(ev->tracks()[0], "Trial.mp3");
}

TEST_F(AOClientTest, ARUPWithInvalidTypeIsIgnored) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<AreaUpdateEvent>();

    // ARUP type 5 is out of valid range (0-3) — handler returns early.
    client->on_message("ARUP#5#val1#val2#%");

    auto ev = ch.get_event();
    EXPECT_FALSE(ev.has_value());
}

TEST_F(AOClientTest, PRRemovePublishesCorrectAction) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<PlayerListEvent>();

    // update_type 1 = remove
    client->on_message("PR#10#1#%");

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->action(), PlayerListEvent::Action::REMOVE);
    EXPECT_EQ(ev->player_id(), 10);
}

TEST_F(AOClientTest, PUInvalidDataTypeIsIgnored) {
    client->on_connect();

    auto& ch = EventManager::instance().get_channel<PlayerListEvent>();

    // data_type 99 is out of valid range — handler returns early.
    client->on_message("PU#0#99#data#%");

    auto ev = ch.get_event();
    EXPECT_FALSE(ev.has_value());
}
