#include <gtest/gtest.h>

#include "game/BanManager.h" // parse_ban_duration

// Tests for the ban duration parser (no BanManager instance needed).

TEST(BanDuration, ParsePerma) {
    EXPECT_EQ(parse_ban_duration("perma"), -2);
    EXPECT_EQ(parse_ban_duration("permanent"), -2);
    EXPECT_EQ(parse_ban_duration("PERMA"), -2);
}

TEST(BanDuration, ParseSeconds) {
    EXPECT_EQ(parse_ban_duration("30s"), 30);
    EXPECT_EQ(parse_ban_duration("60s"), 60);
}

TEST(BanDuration, ParseMinutes) {
    EXPECT_EQ(parse_ban_duration("5m"), 300);
}

TEST(BanDuration, ParseHours) {
    EXPECT_EQ(parse_ban_duration("1h"), 3600);
}

TEST(BanDuration, ParseCompound) {
    EXPECT_EQ(parse_ban_duration("2h30m"), 9000);
    EXPECT_EQ(parse_ban_duration("1h30m15s"), 5415);
}

TEST(BanDuration, ParseBareNumber) {
    EXPECT_EQ(parse_ban_duration("120"), 120);
}

TEST(BanDuration, ParseEmpty) {
    EXPECT_EQ(parse_ban_duration(""), -1);
}

TEST(BanDuration, ParseInvalid) {
    EXPECT_EQ(parse_ban_duration("abc"), -1);
    EXPECT_EQ(parse_ban_duration("5x"), -1);
}
