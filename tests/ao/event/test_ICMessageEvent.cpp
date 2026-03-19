#include "ao/event/ICMessageEvent.h"

#include <gtest/gtest.h>
#include <string>

// ---------------------------------------------------------------------------
// Constructor and getters
// ---------------------------------------------------------------------------

TEST(ICMessageEvent, ConstructorStoresAllFields) {
    ICMessageEvent ev("Phoenix", "normal", "objecting", "def",
                      EmoteMod::PREANIM, DeskMod::SHOW, true, 42);

    EXPECT_EQ(ev.get_character(), "Phoenix");
    EXPECT_EQ(ev.get_emote(), "normal");
    EXPECT_EQ(ev.get_pre_emote(), "objecting");
    EXPECT_EQ(ev.get_side(), "def");
    EXPECT_EQ(ev.get_emote_mod(), EmoteMod::PREANIM);
    EXPECT_EQ(ev.get_desk_mod(), DeskMod::SHOW);
    EXPECT_TRUE(ev.get_flip());
    EXPECT_EQ(ev.get_char_id(), 42);
}

TEST(ICMessageEvent, FlipFalse) {
    ICMessageEvent ev("Edgeworth", "thinking", "", "pro",
                      EmoteMod::IDLE, DeskMod::HIDE, false, 7);
    EXPECT_FALSE(ev.get_flip());
}

TEST(ICMessageEvent, EmptyStrings) {
    ICMessageEvent ev("", "", "", "",
                      EmoteMod::IDLE, DeskMod::HIDE, false, 0);
    EXPECT_EQ(ev.get_character(), "");
    EXPECT_EQ(ev.get_emote(), "");
    EXPECT_EQ(ev.get_pre_emote(), "");
    EXPECT_EQ(ev.get_side(), "");
}

// ---------------------------------------------------------------------------
// Enum integer values
// ---------------------------------------------------------------------------

TEST(EmoteMod, ExpectedIntValues) {
    EXPECT_EQ(static_cast<int>(EmoteMod::IDLE), 0);
    EXPECT_EQ(static_cast<int>(EmoteMod::PREANIM), 1);
    EXPECT_EQ(static_cast<int>(EmoteMod::ZOOM), 5);
    EXPECT_EQ(static_cast<int>(EmoteMod::PREANIM_ZOOM), 6);
}

TEST(DeskMod, ExpectedIntValues) {
    EXPECT_EQ(static_cast<int>(DeskMod::HIDE), 0);
    EXPECT_EQ(static_cast<int>(DeskMod::SHOW), 1);
    EXPECT_EQ(static_cast<int>(DeskMod::EMOTE_ONLY), 2);
    EXPECT_EQ(static_cast<int>(DeskMod::PRE_ONLY), 3);
    EXPECT_EQ(static_cast<int>(DeskMod::EMOTE_ONLY_EX), 4);
    EXPECT_EQ(static_cast<int>(DeskMod::PRE_ONLY_EX), 5);
}

// ---------------------------------------------------------------------------
// to_string
// ---------------------------------------------------------------------------

TEST(ICMessageEvent, ToStringContainsFields) {
    ICMessageEvent ev("Phoenix", "pointing", "objecting", "def",
                      EmoteMod::PREANIM, DeskMod::SHOW, false, 1);
    std::string s = ev.to_string();
    EXPECT_NE(s.find("Phoenix"), std::string::npos);
    EXPECT_NE(s.find("pointing"), std::string::npos);
    EXPECT_NE(s.find("def"), std::string::npos);
}
