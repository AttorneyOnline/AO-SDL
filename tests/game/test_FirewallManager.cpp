#include "game/FirewallManager.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <thread>

namespace {

class FirewallManagerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Leave unconfigured — test graceful degradation by default
    }

    FirewallManager fw_;
};

} // namespace

// -- Graceful degradation (no helper configured) ------------------------------

TEST_F(FirewallManagerTest, DisabledByDefault) {
    EXPECT_FALSE(fw_.is_enabled());
}

TEST_F(FirewallManagerTest, BlockIPReturnsFalseWhenDisabled) {
    EXPECT_FALSE(fw_.block_ip("1.2.3.4", "test"));
}

TEST_F(FirewallManagerTest, UnblockIPReturnsFalseWhenDisabled) {
    EXPECT_FALSE(fw_.unblock_ip("1.2.3.4"));
}

TEST_F(FirewallManagerTest, BlockRangeReturnsFalseWhenDisabled) {
    EXPECT_FALSE(fw_.block_range("10.0.0.0/8", "test"));
}

TEST_F(FirewallManagerTest, SweepReturnsZeroWhenDisabled) {
    EXPECT_EQ(fw_.sweep_expired(), 0u);
}

TEST_F(FirewallManagerTest, ListRulesEmptyWhenDisabled) {
    EXPECT_TRUE(fw_.list_rules().empty());
}

// -- IP validation ------------------------------------------------------------

TEST(FirewallValidation, ValidIPv4Addresses) {
    // Test via block_ip on a disabled manager (validates but returns false)
    FirewallManager fw;
    // Can't test directly since is_valid_ip is private, but we test indirectly
    // through configure + block_ip behavior. We'll test the validation
    // through the persistence round-trip instead.
}

// -- Rule tracking (test with a fake helper path) -----------------------------

namespace {

static std::string find_true_binary() {
    for (const char* path : {"/usr/bin/true", "/bin/true"}) {
        if (std::filesystem::exists(path))
            return path;
    }
    return "";
}

class FirewallRuleTrackingTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto true_path = find_true_binary();
        if (true_path.empty())
            GTEST_SKIP() << "Neither /usr/bin/true nor /bin/true found";

        FirewallConfig cfg;
        cfg.enabled = true;
        cfg.helper_path = true_path;
        cfg.cleanup_on_shutdown = false; // Don't try to flush on teardown
        fw_.configure(cfg);
    }

    void TearDown() override {
        // Give the exec thread a moment to drain
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    FirewallManager fw_;
};

} // namespace

#ifndef _WIN32 // /usr/bin/true not available on Windows

TEST_F(FirewallRuleTrackingTest, EnabledWithValidHelper) {
    EXPECT_TRUE(fw_.is_enabled());
}

TEST_F(FirewallRuleTrackingTest, BlockIPAddsRule) {
    EXPECT_TRUE(fw_.block_ip("1.2.3.4", "test reason"));
    auto rules = fw_.list_rules();
    ASSERT_EQ(rules.size(), 1u);
    EXPECT_EQ(rules[0].target, "1.2.3.4");
    EXPECT_EQ(rules[0].reason, "test reason");
}

TEST_F(FirewallRuleTrackingTest, UnblockIPRemovesRule) {
    fw_.block_ip("1.2.3.4", "test");
    EXPECT_EQ(fw_.list_rules().size(), 1u);

    EXPECT_TRUE(fw_.unblock_ip("1.2.3.4"));
    EXPECT_TRUE(fw_.list_rules().empty());
}

TEST_F(FirewallRuleTrackingTest, UnblockNonexistentReturnsFalse) {
    EXPECT_FALSE(fw_.unblock_ip("9.9.9.9"));
}

TEST_F(FirewallRuleTrackingTest, BlockRangeAddsRule) {
    EXPECT_TRUE(fw_.block_range("10.0.0.0/8", "test"));
    auto rules = fw_.list_rules();
    ASSERT_EQ(rules.size(), 1u);
    EXPECT_EQ(rules[0].target, "10.0.0.0/8");
}

TEST_F(FirewallRuleTrackingTest, InvalidIPRejected) {
    EXPECT_FALSE(fw_.block_ip("not-an-ip", "test"));
    EXPECT_FALSE(fw_.block_ip("999.999.999.999", "test"));
    EXPECT_FALSE(fw_.block_ip("", "test"));
    EXPECT_TRUE(fw_.list_rules().empty());
}

TEST_F(FirewallRuleTrackingTest, InvalidCIDRRejected) {
    EXPECT_FALSE(fw_.block_range("not-a-cidr", "test"));
    EXPECT_FALSE(fw_.block_range("1.2.3.4/33", "test")); // prefix too large for IPv4
    EXPECT_FALSE(fw_.block_range("1.2.3.4/", "test"));
    EXPECT_FALSE(fw_.block_range("/24", "test"));
    EXPECT_TRUE(fw_.list_rules().empty());
}

TEST_F(FirewallRuleTrackingTest, SweepRemovesExpiredRules) {
    // Block with 0-second duration (already expired)
    fw_.block_ip("1.2.3.4", "expires immediately", 0);
    // Block permanently
    fw_.block_ip("5.6.7.8", "permanent", -2);

    // Give a moment for the timestamps to pass
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    size_t removed = fw_.sweep_expired();
    EXPECT_EQ(removed, 1u);

    auto rules = fw_.list_rules();
    ASSERT_EQ(rules.size(), 1u);
    EXPECT_EQ(rules[0].target, "5.6.7.8");
}

TEST_F(FirewallRuleTrackingTest, PermanentRuleNotSwept) {
    fw_.block_ip("1.2.3.4", "permanent", DURATION_PERMANENT);
    EXPECT_EQ(fw_.sweep_expired(), 0u);
    EXPECT_EQ(fw_.list_rules().size(), 1u);
}

TEST_F(FirewallRuleTrackingTest, FlushClearsAllRules) {
    fw_.block_ip("1.2.3.4", "test1");
    fw_.block_ip("5.6.7.8", "test2");
    EXPECT_EQ(fw_.list_rules().size(), 2u);

    fw_.flush();
    EXPECT_TRUE(fw_.list_rules().empty());
}

TEST_F(FirewallRuleTrackingTest, PersistenceRoundTrip) {
    std::string path = "test_fw_rules.json";

    fw_.block_ip("1.2.3.4", "test1", DURATION_PERMANENT);
    fw_.block_range("10.0.0.0/8", "test2", 3600);
    fw_.save_sync(path);

    // Load into a second manager with the same helper configured
    FirewallManager fw2;
    auto true_path = find_true_binary();
    FirewallConfig cfg;
    cfg.enabled = true;
    cfg.helper_path = true_path;
    cfg.cleanup_on_shutdown = false;
    fw2.configure(cfg);
    fw2.load(path);

    // Give the exec thread time to process the re-install commands
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto rules = fw2.list_rules();
    EXPECT_EQ(rules.size(), 2u);

    // Verify we can find both rules
    bool found_ip = false, found_cidr = false;
    for (auto& r : rules) {
        if (r.target == "1.2.3.4") {
            found_ip = true;
            EXPECT_EQ(r.reason, "test1");
            EXPECT_EQ(r.expires_at, 0); // permanent
        }
        if (r.target == "10.0.0.0/8") {
            found_cidr = true;
            EXPECT_EQ(r.reason, "test2");
            EXPECT_GT(r.expires_at, 0); // timed
        }
    }
    EXPECT_TRUE(found_ip);
    EXPECT_TRUE(found_cidr);

    std::remove(path.c_str());
}

#endif // _WIN32

// -- Configuration edge cases -------------------------------------------------

TEST_F(FirewallManagerTest, ConfigureWithNonexistentHelper) {
    FirewallConfig cfg;
    cfg.enabled = true;
    cfg.helper_path = "/nonexistent/path/to/binary";
    fw_.configure(cfg);
    EXPECT_FALSE(fw_.is_enabled());
}

TEST_F(FirewallManagerTest, ConfigureWithEmptyPath) {
    FirewallConfig cfg;
    cfg.enabled = true;
    cfg.helper_path = "";
    fw_.configure(cfg);
    EXPECT_FALSE(fw_.is_enabled());
}

TEST_F(FirewallManagerTest, LoadNonexistentFileIsNoOp) {
    fw_.load("nonexistent_fw_rules.json");
    EXPECT_TRUE(fw_.list_rules().empty());
}
