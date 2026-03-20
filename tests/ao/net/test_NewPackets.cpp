#include "ao/event/ICMessageEvent.h"
#include "ao/net/AOClient.h"
#include "ao/net/AOPacket.h"
#include "ao/net/PacketTypes.h"
#include "event/BackgroundEvent.h"
#include "event/EventManager.h"

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
