#include "event/CharacterListEvent.h"
#include "event/CharsCheckEvent.h"
#include "event/ChatEvent.h"
#include "event/EventManager.h"
#include "event/UIEvent.h"
#include "net/ao/AOClient.h"
#include "net/ao/AOPacket.h"
#include "net/ao/PacketTypes.h"

#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <vector>

// Drain an event channel so test isolation is maintained.
template <typename T>
static void drain(EventChannel<T>& ch) {
    while (ch.get_event()) {
    }
}

// ---------------------------------------------------------------------------
// AOPacketHI — serialize
// ---------------------------------------------------------------------------

TEST(AOPacketHI, SerializesCorrectly) {
    AOPacketHI hi("myhwid");
    EXPECT_EQ(hi.serialize(), "HI#myhwid#%");
}

// ---------------------------------------------------------------------------
// AOPacketCT — construct and serialize
// ---------------------------------------------------------------------------

TEST(AOPacketCT, SerializesWithSystemFlag) {
    AOPacketCT ct("Sys", "Welcome", true);
    EXPECT_EQ(ct.serialize(), "CT#Sys#Welcome#1#%");
}

TEST(AOPacketCT, SerializesWithoutSystemFlag) {
    AOPacketCT ct("Phoenix", "Objection!", false);
    EXPECT_EQ(ct.serialize(), "CT#Phoenix#Objection!#0#%");
}

TEST(AOPacketCT, HandlePublishesChatEvent) {
    auto& ch = EventManager::instance().get_channel<ChatEvent>();
    drain(ch);

    AOClient cli;
    cli.conn_state = CONNECTED;

    AOPacketCT ct(std::vector<std::string>{"Phoenix", "Objection!", "0"});
    ct.handle(cli);

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    drain(ch);
}

// ---------------------------------------------------------------------------
// AOPacketPW — serialize
// ---------------------------------------------------------------------------

TEST(AOPacketPW, SerializesEmptyPassword) {
    AOPacketPW pw("");
    EXPECT_EQ(pw.serialize(), "PW##%");
}

TEST(AOPacketPW, SerializesWithPassword) {
    AOPacketPW pw("hunter2");
    EXPECT_EQ(pw.serialize(), "PW#hunter2#%");
}

// ---------------------------------------------------------------------------
// AOPacketCC — serialize
// ---------------------------------------------------------------------------

TEST(AOPacketCC, SerializesFields) {
    AOPacketCC cc(1, 3, "myhwid");
    EXPECT_EQ(cc.serialize(), "CC#1#3#myhwid#%");
}

// ---------------------------------------------------------------------------
// AOPacketCharsCheck — parsing "-1" as taken
// ---------------------------------------------------------------------------

TEST(AOPacketCharsCheck, ParsesTakenFlags) {
    // "-1" means taken, "0" means free.
    AOPacketCharsCheck cc({"-1", "0", "-1", "0", "0"});

    auto& ch = EventManager::instance().get_channel<CharsCheckEvent>();
    drain(ch);

    AOClient cli;
    cc.handle(cli);

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    const auto& taken = ev->get_taken();
    ASSERT_EQ(taken.size(), 5u);
    EXPECT_TRUE(taken[0]);
    EXPECT_FALSE(taken[1]);
    EXPECT_TRUE(taken[2]);
    EXPECT_FALSE(taken[3]);
    EXPECT_FALSE(taken[4]);
    drain(ch);
}

TEST(AOPacketCharsCheck, AllFreeFlags) {
    AOPacketCharsCheck cc({"0", "0", "0"});
    auto& ch = EventManager::instance().get_channel<CharsCheckEvent>();
    drain(ch);
    AOClient cli;
    cc.handle(cli);
    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    for (bool t : ev->get_taken()) {
        EXPECT_FALSE(t);
    }
    drain(ch);
}

// ---------------------------------------------------------------------------
// AOPacketPV — publishes ENTERED_COURTROOM
// ---------------------------------------------------------------------------

TEST(AOPacketPV, HandlePublishesEnteredCourtroomEvent) {
    auto& ch = EventManager::instance().get_channel<UIEvent>();
    drain(ch);

    AOClient cli;
    cli.conn_state = CONNECTED;

    AOPacketPV pv({"1", "CID", "2"});
    pv.handle(cli);

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_type(), ENTERED_COURTROOM);
    drain(ch);
}

// ---------------------------------------------------------------------------
// AOPacketDONE — publishes CHAR_LOADING_DONE and sets state
// ---------------------------------------------------------------------------

TEST(AOPacketDONE, HandlePublishesCharLoadingDone) {
    auto& ch = EventManager::instance().get_channel<UIEvent>();
    drain(ch);

    AOClient cli;
    cli.conn_state = CONNECTED;

    AOPacketDONE done;
    done.handle(cli);

    EXPECT_EQ(cli.conn_state, JOINED);

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_type(), CHAR_LOADING_DONE);
    drain(ch);
}

// ---------------------------------------------------------------------------
// AOPacketSC — publishes CharacterListEvent with folder names
// ---------------------------------------------------------------------------

TEST(AOPacketSC, HandlePublishesFolderNames) {
    auto& ch = EventManager::instance().get_channel<CharacterListEvent>();
    drain(ch);

    AOClient cli;
    cli.conn_state = CONNECTED;

    // Each field is "folder_name&display_name".
    AOPacketSC sc({"Phoenix&Phoenix Wright", "Edgeworth&Miles Edgeworth"});
    sc.handle(cli);

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    const auto& names = ev->get_characters();
    ASSERT_EQ(names.size(), 2u);
    EXPECT_EQ(names[0], "Phoenix");
    EXPECT_EQ(names[1], "Edgeworth");
    drain(ch);
}

// ---------------------------------------------------------------------------
// Deserialization via PacketFactory (registrar-driven)
// ---------------------------------------------------------------------------

TEST(AOPacketDeserialize, ProducesKnownDerivedTypes) {
    // HI round-trip — deserialize should still produce the correct wire bytes.
    auto pkt = AOPacket::deserialize("HI#testhwid#%");
    ASSERT_NE(pkt, nullptr);
    EXPECT_EQ(pkt->serialize(), "HI#testhwid#%");
}

TEST(AOPacketDeserialize, CharsCheckPacket) {
    auto pkt = AOPacket::deserialize("CharsCheck#-1#0#-1#%");
    ASSERT_NE(pkt, nullptr);
    EXPECT_EQ(pkt->serialize(), "CharsCheck#-1#0#-1#%");
}
