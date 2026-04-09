#include <gtest/gtest.h>

#include "game/BanManager.h"

#include <chrono>
#include <memory>
#include <thread>

// BanManager starts a background jthread in its constructor.
// Use a unique_ptr and explicit destruction to avoid shutdown races
// in the test harness.

namespace {

int64_t now_seconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

BanEntry make_ban(const std::string& ipid, const std::string& hdid = "", int64_t duration = -2) {
    BanEntry ban;
    ban.ipid = ipid;
    ban.hdid = hdid;
    ban.duration = duration;
    ban.timestamp = now_seconds();
    return ban;
}

class BanManagerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        bm_ = std::make_unique<BanManager>();
    }

    void TearDown() override {
        bm_.reset();
        // Allow background thread to fully wind down.
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    std::unique_ptr<BanManager> bm_;
};

} // namespace

TEST_F(BanManagerTest, AddAndFind) {
    auto ban = make_ban("abc123", "hw456");
    ban.reason = "test ban";
    ban.moderator = "admin";
    bm_->add_ban(ban);

    auto found = bm_->find_ban("abc123");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->ipid, "abc123");
    EXPECT_EQ(found->reason, "test ban");
    EXPECT_TRUE(found->is_permanent());
}

TEST_F(BanManagerTest, FindByHdid) {
    bm_->add_ban(make_ban("ip1", "unique_hw"));

    auto found = bm_->find_ban_by_hdid("unique_hw");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->hdid, "unique_hw");
}

TEST_F(BanManagerTest, CheckBan_IpidFirst) {
    bm_->add_ban(make_ban("banned_ip", "banned_hw"));

    auto found = bm_->check_ban("banned_ip", "other_hw");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->ipid, "banned_ip");
}

TEST_F(BanManagerTest, CheckBan_FallsBackToHdid) {
    bm_->add_ban(make_ban("some_ip", "banned_hw"));

    auto found = bm_->check_ban("other_ip", "banned_hw");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->hdid, "banned_hw");
}

TEST_F(BanManagerTest, NotFound) {
    EXPECT_FALSE(bm_->find_ban("nonexistent").has_value());
    EXPECT_FALSE(bm_->find_ban_by_hdid("nonexistent").has_value());
    EXPECT_FALSE(bm_->check_ban("x", "y").has_value());
}

TEST_F(BanManagerTest, RemoveBan) {
    bm_->add_ban(make_ban("removeme"));
    EXPECT_TRUE(bm_->remove_ban("removeme"));
    EXPECT_FALSE(bm_->find_ban("removeme").has_value());
}

TEST_F(BanManagerTest, RemoveBan_NotFound) {
    EXPECT_FALSE(bm_->remove_ban("ghost"));
}

TEST_F(BanManagerTest, OverwriteByIpid) {
    auto ban1 = make_ban("same_ip");
    ban1.reason = "first";
    bm_->add_ban(ban1);

    auto ban2 = make_ban("same_ip");
    ban2.reason = "second";
    bm_->add_ban(ban2);

    auto found = bm_->find_ban("same_ip");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->reason, "second");
    EXPECT_EQ(bm_->count(), 1u);
}

TEST_F(BanManagerTest, ExpiredBanNotFound) {
    BanEntry ban;
    ban.ipid = "expired";
    ban.timestamp = 1;
    ban.duration = 1;
    bm_->add_ban(ban);

    EXPECT_FALSE(bm_->find_ban("expired").has_value());
    EXPECT_FALSE(bm_->check_ban("expired", "").has_value());
}

TEST_F(BanManagerTest, CountExcludesExpired) {
    bm_->add_ban(make_ban("active"));

    BanEntry expired;
    expired.ipid = "expired";
    expired.timestamp = 1;
    expired.duration = 1;
    bm_->add_ban(expired);

    EXPECT_EQ(bm_->count(), 1u);
}

// -- Ban duration parsing -----------------------------------------------------

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
