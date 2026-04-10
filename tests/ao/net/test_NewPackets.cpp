#include "event/BackgroundEvent.h"
#include "event/EventManager.h"
#include "event/ICMessageEvent.h"
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

// Helper: build a minimal 15-field MS packet vector.
// Fields: 0=desk_mod, 1=pre_emote, 2=character, 3=emote, 4=message,
//         5=side, 6=sfx_name, 7=emote_mod, 8=char_id, 9=sfx_delay,
//         10=objection_mod, 11=evidence_id, 12=flip, 13=realization, 14=text_color
static std::vector<std::string>
make_ms_fields(const std::string& desk_mod = "1", const std::string& pre_emote = "objecting",
               const std::string& character = "Phoenix", const std::string& emote = "normal",
               const std::string& message = "Objection!", const std::string& side = "def",
               const std::string& sfx_name = "sfx-objection", const std::string& emote_mod = "1",
               const std::string& char_id = "42", const std::string& sfx_delay = "0", const std::string& obj_mod = "0",
               const std::string& evidence_id = "0", const std::string& flip = "0",
               const std::string& realization = "0", const std::string& text_color = "0") {
    return {desk_mod, pre_emote, character, emote,       message, side,        sfx_name,  emote_mod,
            char_id,  sfx_delay, obj_mod,   evidence_id, flip,    realization, text_color};
}

// ===========================================================================
// AOPacketBN
// ===========================================================================

TEST(AOPacketBN, ParsesBackgroundName) {
    AOPacketBN bn({"gs4", "def"});
    EXPECT_EQ(bn.serialize(), "BN#gs4#def#%");
}

TEST(AOPacketBN, ParsesBackgroundOnly) {
    AOPacketBN bn({"default"});
    EXPECT_EQ(bn.serialize(), "BN#default#%");
}

TEST(AOPacketBN, HandlePublishesBackgroundEvent) {
    auto& ch = EventManager::instance().get_channel<BackgroundEvent>();
    drain(ch);

    AOClient cli;
    cli.conn_state = CONNECTED;

    AOPacketBN bn({"gs4", "pro"});
    bn.handle(cli);

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_background(), "gs4");
    EXPECT_EQ(ev->get_position(), "pro");
    drain(ch);
}

TEST(AOPacketBN, HandlePublishesEmptyPositionWhenSingleField) {
    auto& ch = EventManager::instance().get_channel<BackgroundEvent>();
    drain(ch);

    AOClient cli;
    cli.conn_state = CONNECTED;

    AOPacketBN bn({"courtroom"});
    bn.handle(cli);

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_background(), "courtroom");
    EXPECT_EQ(ev->get_position(), "");
    drain(ch);
}

TEST(AOPacketBN, DeserializeRoundTrip) {
    auto pkt = AOPacket::deserialize("BN#night#wit#%");
    ASSERT_NE(pkt, nullptr);
    EXPECT_EQ(pkt->serialize(), "BN#night#wit#%");
}

// ===========================================================================
// AOPacketMS — field parsing
// ===========================================================================

TEST(AOPacketMS, ParsesCoreFields) {
    auto fields = make_ms_fields("1", "objecting", "Phoenix", "normal", "Hello", "def", "sfx", "1", "42", "0", "0", "0",
                                 "1", "0", "0");
    AOPacketMS ms(fields);
    // Verify round-trip serialization preserves all 15 fields.
    EXPECT_EQ(ms.serialize(), "MS#1#objecting#Phoenix#normal#Hello#def#sfx#1#42#0#0#0#1#0#0#%");
}

// ---------------------------------------------------------------------------
// Legacy emote_mod remapping
// ---------------------------------------------------------------------------

TEST(AOPacketMS, LegacyEmoteMod4RemapsTo6) {
    auto& ch = EventManager::instance().get_channel<ICMessageEvent>();
    drain(ch);

    AOClient cli;
    cli.conn_state = CONNECTED;

    auto fields = make_ms_fields("0", "pre", "Char", "em", "msg", "def", "sfx", "4", "1", "0", "0", "0", "0", "0", "0");
    AOPacketMS ms(fields);
    ms.handle(cli);

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_emote_mod(), EmoteMod::PREANIM_ZOOM);
    drain(ch);
}

TEST(AOPacketMS, LegacyEmoteMod2RemapsTo1) {
    auto& ch = EventManager::instance().get_channel<ICMessageEvent>();
    drain(ch);

    AOClient cli;
    cli.conn_state = CONNECTED;

    auto fields = make_ms_fields("0", "pre", "Char", "em", "msg", "def", "sfx", "2", "1", "0", "0", "0", "0", "0", "0");
    AOPacketMS ms(fields);
    ms.handle(cli);

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_emote_mod(), EmoteMod::PREANIM);
    drain(ch);
}

TEST(AOPacketMS, InvalidEmoteModDefaultsToIdle) {
    auto& ch = EventManager::instance().get_channel<ICMessageEvent>();
    drain(ch);

    AOClient cli;
    cli.conn_state = CONNECTED;

    auto fields =
        make_ms_fields("0", "pre", "Char", "em", "msg", "def", "sfx", "99", "1", "0", "0", "0", "0", "0", "0");
    AOPacketMS ms(fields);
    ms.handle(cli);

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_emote_mod(), EmoteMod::IDLE);
    drain(ch);
}

// ---------------------------------------------------------------------------
// AOPacketMS — handle() publishes ICMessageEvent with correct fields
// ---------------------------------------------------------------------------

TEST(AOPacketMS, HandlePublishesICMessageEvent) {
    auto& ch = EventManager::instance().get_channel<ICMessageEvent>();
    drain(ch);

    AOClient cli;
    cli.conn_state = CONNECTED;

    auto fields = make_ms_fields("1", "objecting", "Phoenix", "pointing", "Take that!", "def", "sfx-objection", "1",
                                 "42", "0", "0", "0", "1", "0", "0");
    AOPacketMS ms(fields);
    ms.handle(cli);

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_character(), "Phoenix");
    EXPECT_EQ(ev->get_emote(), "pointing");
    EXPECT_EQ(ev->get_pre_emote(), "objecting");
    EXPECT_EQ(ev->get_side(), "def");
    EXPECT_EQ(ev->get_emote_mod(), EmoteMod::PREANIM);
    EXPECT_EQ(ev->get_desk_mod(), DeskMod::SHOW);
    EXPECT_TRUE(ev->get_flip());
    EXPECT_EQ(ev->get_char_id(), 42);
    drain(ch);
}

TEST(AOPacketMS, HandleFlipFalseWhenFieldIsZero) {
    auto& ch = EventManager::instance().get_channel<ICMessageEvent>();
    drain(ch);

    AOClient cli;
    cli.conn_state = CONNECTED;

    auto fields = make_ms_fields("0", "pre", "Edgeworth", "thinking", "...", "pro", "sfx", "0", "7", "0", "0", "0", "0",
                                 "0", "0");
    AOPacketMS ms(fields);
    ms.handle(cli);

    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_FALSE(ev->get_flip());
    EXPECT_EQ(ev->get_desk_mod(), DeskMod::HIDE);
    EXPECT_EQ(ev->get_emote_mod(), EmoteMod::IDLE);
    drain(ch);
}

TEST(AOPacketMS, DeserializeRoundTrip) {
    auto pkt = AOPacket::deserialize("MS#1#objecting#Phoenix#normal#Hello#def#sfx#1#42#0#0#0#0#0#0#%");
    ASSERT_NE(pkt, nullptr);
    EXPECT_EQ(pkt->serialize(), "MS#1#objecting#Phoenix#normal#Hello#def#sfx#1#42#0#0#0#0#0#0#%");
}

// Regression: webAO parses a malformed [SoundT] block in char.ini as
// `parseInt("02 = 03 = ...") === NaN`, so the MS packet ships the literal
// string "NaN" in field 9 (sfx_delay). Before the safe_stoi refactor, the
// AOPacketMS constructor hit `std::stoi("NaN")` and threw, and because
// AOServer::on_client_message called AOPacket::deserialize *outside* its
// try/catch, the exception tore through the worker thread and crashed the
// whole kagami process. Both halves of that defense are now covered:
//   1. AOPacketMS constructor recovers gracefully from non-numeric fields
//      (sfx_delay defaults to 0 — see safe_stoi in PacketTypes.cpp).
//   2. ao_packet::safe_dispatch catches any exception that still escapes
//      (see test_AOPacket.cpp::AOPacketSafeDispatch).
TEST(AOPacketMS, DeserializeWebaoNaNSfxDelayDoesNotThrow) {
    auto& ch = EventManager::instance().get_channel<ICMessageEvent>();
    drain(ch);

    AOClient cli;
    cli.conn_state = CONNECTED;

    constexpr const char* webao_wire = "MS#1#-#neuro#/normal#test#def#02  = 03  = 04  = 05  = 06  = 07  = 08  = "
                                       "0#0#40#NaN#0#0#0#0#0##-1#0#0#0#0#-#-#-#0#||#%";

    // Constructor must not throw even though field 9 ("NaN") is non-numeric.
    // safe_stoi in PacketTypes.cpp recovers with 0.
    std::unique_ptr<AOPacket> pkt;
    ASSERT_NO_THROW(pkt = AOPacket::deserialize(webao_wire));
    ASSERT_NE(pkt, nullptr);
    EXPECT_EQ(pkt->get_header(), "MS");

    // Dispatching to a client must also succeed and publish a valid IC event
    // with the fields that parsed cleanly.
    pkt->handle(cli);
    auto ev = ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_character(), "neuro");
    EXPECT_EQ(ev->get_emote(), "/normal");
    EXPECT_EQ(ev->get_char_id(), 40);
    drain(ch);
}
