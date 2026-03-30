#include "ui/widgets/CourtroomState.h"

#include <gtest/gtest.h>

TEST(CourtroomState, Singleton_ReturnsSameInstance) {
    auto& a = CourtroomState::instance();
    auto& b = CourtroomState::instance();
    EXPECT_EQ(&a, &b);
}

TEST(CourtroomState, Reset_ClearsAllFields) {
    auto& cs = CourtroomState::instance();

    // Populate everything
    cs.areas = {"Lobby", "Courtroom"};
    cs.tracks = {"Track1", "Track2"};
    cs.area_players = {5, 3};
    cs.area_status = {"CASING", "RECESS"};
    cs.area_cm = {"user1", "FREE"};
    cs.area_lock = {"FREE", "LOCKED"};
    cs.now_playing = "some track";
    cs.evidence = {{"Item", "desc", "img"}};
    cs.players[1] = {"name", "char", "charname", 0};
    cs.def_hp = 7;
    cs.pro_hp = 4;
    cs.chat_log.push_back({"Alice", "Hello", false});

    cs.reset();

    EXPECT_TRUE(cs.areas.empty());
    EXPECT_TRUE(cs.tracks.empty());
    EXPECT_TRUE(cs.area_players.empty());
    EXPECT_TRUE(cs.area_status.empty());
    EXPECT_TRUE(cs.area_cm.empty());
    EXPECT_TRUE(cs.area_lock.empty());
    EXPECT_TRUE(cs.now_playing.empty());
    EXPECT_TRUE(cs.evidence.empty());
    EXPECT_TRUE(cs.players.empty());
    EXPECT_EQ(cs.def_hp, 0);
    EXPECT_EQ(cs.pro_hp, 0);
    EXPECT_TRUE(cs.chat_log.empty());
}
