#include "game/ASNReputationManager.h"
#include "game/FirewallManager.h" // DURATION_PERMANENT

#include <gtest/gtest.h>

#include <cstdio>

namespace {

class ASNReputationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        ASNReputationConfig cfg;
        cfg.enabled = true;
        cfg.watch_threshold = 2;
        cfg.rate_limit_threshold = 3;
        cfg.block_threshold = 5;
        cfg.window_minutes = 60;
        cfg.auto_block_duration = "1h";
        mgr_.configure(cfg);
    }

    ASNReputationManager mgr_;
};

} // namespace

TEST_F(ASNReputationTest, NoStatusForUnknownASN) {
    EXPECT_FALSE(mgr_.get_status(99999).has_value());
}

TEST_F(ASNReputationTest, SingleAbuseCreatesEntry) {
    mgr_.report_abuse(12345, "1.2.3.4", "TestOrg", "test");
    auto status = mgr_.get_status(12345);
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->asn, 12345u);
    EXPECT_EQ(status->as_org, "TestOrg");
    EXPECT_EQ(status->total_abuse_events, 1);
}

TEST_F(ASNReputationTest, EscalatesToWatched) {
    // Threshold is 2 unique IPs
    mgr_.report_abuse(100, "1.1.1.1", "Org", "spam");
    EXPECT_EQ(mgr_.get_status(100)->status, ASNReputationEntry::Status::NORMAL);

    mgr_.report_abuse(100, "2.2.2.2", "Org", "spam");
    EXPECT_EQ(mgr_.get_status(100)->status, ASNReputationEntry::Status::WATCHED);
}

TEST_F(ASNReputationTest, EscalatesToRateLimited) {
    // Threshold is 3 unique IPs
    mgr_.report_abuse(100, "1.1.1.1", "Org", "spam");
    mgr_.report_abuse(100, "2.2.2.2", "Org", "spam");
    mgr_.report_abuse(100, "3.3.3.3", "Org", "spam");
    EXPECT_EQ(mgr_.get_status(100)->status, ASNReputationEntry::Status::RATE_LIMITED);
}

TEST_F(ASNReputationTest, EscalatesToBlocked) {
    // Threshold is 5 unique IPs
    for (int i = 1; i <= 5; ++i) {
        std::string ip = std::to_string(i) + ".0.0.1";
        mgr_.report_abuse(100, ip, "Org", "spam");
    }
    EXPECT_EQ(mgr_.get_status(100)->status, ASNReputationEntry::Status::BLOCKED);
}

TEST_F(ASNReputationTest, DuplicateIPDoesNotCountTwice) {
    mgr_.report_abuse(100, "1.1.1.1", "Org", "spam");
    mgr_.report_abuse(100, "1.1.1.1", "Org", "spam again");
    // Still only 1 unique IP — should stay NORMAL
    EXPECT_EQ(mgr_.get_status(100)->status, ASNReputationEntry::Status::NORMAL);
    EXPECT_EQ(mgr_.get_status(100)->total_abuse_events, 2);
}

TEST_F(ASNReputationTest, CheckBlockedReturnsTrueWhenBlocked) {
    mgr_.block_asn(200, "BadOrg", "manual block", DURATION_PERMANENT);
    auto blocked = mgr_.check_blocked(200);
    ASSERT_TRUE(blocked.has_value());
    EXPECT_EQ(blocked->block_reason, "manual block");
}

TEST_F(ASNReputationTest, CheckBlockedReturnsFalseWhenNormal) {
    mgr_.report_abuse(100, "1.1.1.1", "Org", "spam");
    EXPECT_FALSE(mgr_.check_blocked(100).has_value());
}

TEST_F(ASNReputationTest, UnblockResetsToNormal) {
    mgr_.block_asn(200, "BadOrg", "test", -2);
    EXPECT_TRUE(mgr_.check_blocked(200).has_value());

    EXPECT_TRUE(mgr_.unblock_asn(200));
    EXPECT_FALSE(mgr_.check_blocked(200).has_value());
    EXPECT_EQ(mgr_.get_status(200)->status, ASNReputationEntry::Status::NORMAL);
}

TEST_F(ASNReputationTest, UnblockNonexistentReturnsFalse) {
    EXPECT_FALSE(mgr_.unblock_asn(99999));
}

TEST_F(ASNReputationTest, ListFlaggedReturnsOnlyNonNormal) {
    mgr_.report_abuse(100, "1.1.1.1", "Org1", "spam");
    mgr_.report_abuse(100, "2.2.2.2", "Org1", "spam"); // → WATCHED
    mgr_.report_abuse(200, "3.3.3.3", "Org2", "spam");  // NORMAL (only 1 IP)

    auto flagged = mgr_.list_flagged();
    EXPECT_EQ(flagged.size(), 1u);
    EXPECT_EQ(flagged[0].asn, 100u);
}

TEST_F(ASNReputationTest, WhitelistMultipliesThresholds) {
    ASNReputationConfig cfg;
    cfg.enabled = true;
    cfg.watch_threshold = 2;
    cfg.rate_limit_threshold = 3;
    cfg.block_threshold = 5;
    cfg.whitelist_asns = {7922}; // "Comcast"
    cfg.whitelist_multiplier = 5;
    cfg.window_minutes = 60;
    cfg.auto_block_duration = "1h";
    mgr_.configure(cfg);

    // 2 unique IPs should NOT escalate whitelisted ASN (threshold = 2*5 = 10)
    mgr_.report_abuse(7922, "1.1.1.1", "Comcast", "spam");
    mgr_.report_abuse(7922, "2.2.2.2", "Comcast", "spam");
    EXPECT_EQ(mgr_.get_status(7922)->status, ASNReputationEntry::Status::NORMAL);

    // 10 unique IPs should escalate to WATCHED
    for (int i = 3; i <= 10; ++i) {
        std::string ip = std::to_string(i) + ".0.0.1";
        mgr_.report_abuse(7922, ip, "Comcast", "spam");
    }
    EXPECT_EQ(mgr_.get_status(7922)->status, ASNReputationEntry::Status::WATCHED);
}

TEST_F(ASNReputationTest, StatusCallbackInvokedOnEscalation) {
    int callback_count = 0;
    uint32_t callback_asn = 0;
    ASNReputationEntry::Status callback_new_status{};

    mgr_.set_status_callback(
        [&](uint32_t asn, ASNReputationEntry::Status, ASNReputationEntry::Status new_status, const std::string&) {
            ++callback_count;
            callback_asn = asn;
            callback_new_status = new_status;
        });

    mgr_.report_abuse(100, "1.1.1.1", "Org", "spam");
    EXPECT_EQ(callback_count, 0); // No escalation yet

    mgr_.report_abuse(100, "2.2.2.2", "Org", "spam"); // → WATCHED
    EXPECT_EQ(callback_count, 1);
    EXPECT_EQ(callback_asn, 100u);
    EXPECT_EQ(callback_new_status, ASNReputationEntry::Status::WATCHED);
}

TEST_F(ASNReputationTest, PersistenceRoundTrip) {
    std::string path = "test_asn_rep.json";

    mgr_.block_asn(200, "BadOrg", "test block", -2);
    mgr_.report_abuse(100, "1.1.1.1", "Org", "spam");
    mgr_.report_abuse(100, "2.2.2.2", "Org", "spam"); // → WATCHED
    mgr_.save_sync(path);

    ASNReputationManager mgr2;
    mgr2.configure({.enabled = true});
    mgr2.load(path);

    auto s100 = mgr2.get_status(100);
    ASSERT_TRUE(s100.has_value());
    EXPECT_EQ(s100->status, ASNReputationEntry::Status::WATCHED);

    auto s200 = mgr2.get_status(200);
    ASSERT_TRUE(s200.has_value());
    EXPECT_EQ(s200->status, ASNReputationEntry::Status::BLOCKED);
    EXPECT_EQ(s200->block_reason, "test block");

    std::remove(path.c_str());
}

TEST_F(ASNReputationTest, ZeroASNIgnored) {
    mgr_.report_abuse(0, "1.1.1.1", "Org", "spam");
    EXPECT_EQ(mgr_.count(), 0u);
}

TEST_F(ASNReputationTest, CountTracksEntries) {
    EXPECT_EQ(mgr_.count(), 0u);
    mgr_.report_abuse(100, "1.1.1.1", "Org", "spam");
    EXPECT_EQ(mgr_.count(), 1u);
    mgr_.report_abuse(200, "2.2.2.2", "Org2", "spam");
    EXPECT_EQ(mgr_.count(), 2u);
}

TEST(ASNStatusString, AllValuesHaveStrings) {
    EXPECT_STREQ(asn_status_to_string(ASNReputationEntry::Status::NORMAL), "NORMAL");
    EXPECT_STREQ(asn_status_to_string(ASNReputationEntry::Status::WATCHED), "WATCHED");
    EXPECT_STREQ(asn_status_to_string(ASNReputationEntry::Status::RATE_LIMITED), "RATE_LIMITED");
    EXPECT_STREQ(asn_status_to_string(ASNReputationEntry::Status::BLOCKED), "BLOCKED");
}
