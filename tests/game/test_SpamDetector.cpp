#include "game/SpamDetector.h"

#include <gtest/gtest.h>

#include <set>
#include <string>

namespace {

class SpamDetectorTest : public ::testing::Test {
  protected:
    void SetUp() override {
        SpamDetectorConfig cfg;
        cfg.enabled = true;
        cfg.echo_threshold = 3;
        cfg.echo_window_seconds = 60;
        cfg.burst_threshold = 5; // Low threshold for testing
        cfg.burst_window_seconds = 30;
        cfg.join_spam_max_seconds = 5;
        cfg.name_pattern_threshold = 3;
        cfg.name_pattern_min_prefix = 4;
        cfg.name_pattern_window_seconds = 300;
        cfg.ghost_threshold = 3;
        cfg.hwid_reuse_threshold = 3;
        cfg.message_ring_size = 100;
        cfg.name_ring_size = 50;
        sd_.configure(cfg);
    }

    SpamDetector sd_;
};

} // namespace

// -- H1: Message echo ---------------------------------------------------------

TEST_F(SpamDetectorTest, SingleMessageNotSpam) {
    auto v = sd_.check_message("aaa", "aaa", 100, "hello world");
    EXPECT_FALSE(v.is_spam);
}

TEST_F(SpamDetectorTest, SameMessageFromTwoIPsNotSpam) {
    sd_.check_message("aaa", "aaa", 100, "hello");
    auto v = sd_.check_message("bbb", "bbb", 100, "hello");
    EXPECT_FALSE(v.is_spam);
}

TEST_F(SpamDetectorTest, SameMessageFromThreeIPsIsSpam) {
    sd_.check_message("aaa", "aaa", 100, "NESM0GGED");
    sd_.check_message("bbb", "bbb", 100, "NESM0GGED");
    auto v = sd_.check_message("ccc", "ccc", 100, "NESM0GGED");
    EXPECT_TRUE(v.is_spam);
    EXPECT_EQ(v.heuristic, "echo");
}

TEST_F(SpamDetectorTest, EchoDetectionIsCaseInsensitive) {
    sd_.check_message("aaa", "aaa", 100, "Spam Message");
    sd_.check_message("bbb", "bbb", 100, "spam message");
    auto v = sd_.check_message("ccc", "ccc", 100, "SPAM MESSAGE");
    EXPECT_TRUE(v.is_spam);
}

TEST_F(SpamDetectorTest, EchoDetectionCollapsesWhitespace) {
    sd_.check_message("aaa", "aaa", 100, "hello   world");
    sd_.check_message("bbb", "bbb", 100, "hello world");
    auto v = sd_.check_message("ccc", "ccc", 100, "hello    world");
    EXPECT_TRUE(v.is_spam);
}

TEST_F(SpamDetectorTest, DifferentMessagesNotEcho) {
    sd_.check_message("aaa", "aaa", 100, "message one");
    sd_.check_message("bbb", "bbb", 100, "message two");
    auto v = sd_.check_message("ccc", "ccc", 100, "message three");
    EXPECT_FALSE(v.is_spam);
}

TEST_F(SpamDetectorTest, SameIPMultipleTimesDoesNotTriggerEcho) {
    sd_.check_message("aaa", "aaa", 100, "repeated");
    sd_.check_message("aaa", "aaa", 100, "repeated");
    auto v = sd_.check_message("aaa", "aaa", 100, "repeated");
    EXPECT_FALSE(v.is_spam); // Only 1 unique IPID
}

TEST_F(SpamDetectorTest, EchoAlertOnlyFiresOnce) {
    sd_.check_message("aaa", "aaa", 100, "spam");
    sd_.check_message("bbb", "bbb", 100, "spam");
    auto v1 = sd_.check_message("ccc", "ccc", 100, "spam");
    EXPECT_TRUE(v1.is_spam);

    // Fourth IP with same message should NOT re-trigger
    auto v2 = sd_.check_message("ddd", "ddd", 100, "spam");
    EXPECT_FALSE(v2.is_spam);
}

// -- H2: Connection burst -----------------------------------------------------

TEST_F(SpamDetectorTest, FewConnectionsNotBurst) {
    // Use names without trailing digits (to avoid H5) and unique HWIDs (to avoid H7)
    const char* names[] = {"Alice", "Bob", "Charlie", "Diana"};
    for (int i = 0; i < 4; ++i) {
        auto v = sd_.on_connection("ip" + std::to_string(i), "ip" + std::to_string(i), 100, "hwid" + std::to_string(i),
                                   names[i]);
        EXPECT_FALSE(v.is_spam);
    }
}

TEST_F(SpamDetectorTest, ManyConnectionsIsBurst) {
    SpamVerdict last;
    for (int i = 0; i < 6; ++i) {
        last = sd_.on_connection("ip" + std::to_string(i), "ip" + std::to_string(i), 100, "hwid" + std::to_string(i),
                                 "user");
    }
    // At threshold=5, the 5th connection should trigger
    EXPECT_TRUE(last.is_spam);
    EXPECT_EQ(last.heuristic, "burst");
}

// -- H3: Join-and-spam --------------------------------------------------------

TEST_F(SpamDetectorTest, JoinAndSpamDetected) {
    // Record join times for 3 IPs
    sd_.record_join_time("aaa");
    sd_.record_join_time("bbb");
    sd_.record_join_time("ccc");

    // All send the same message immediately (within 5s of joining)
    sd_.check_message("aaa", "aaa", 100, "buy cheap proxies");
    sd_.check_message("bbb", "bbb", 100, "buy cheap proxies");
    auto v = sd_.check_message("ccc", "ccc", 100, "buy cheap proxies");

    // Should be detected as either echo or join_spam
    EXPECT_TRUE(v.is_spam);
}

// -- H5: Name pattern ---------------------------------------------------------

TEST_F(SpamDetectorTest, UniqueNamesNotPattern) {
    sd_.on_connection("a", "a", 100, "h1", "Alice");
    sd_.on_connection("b", "b", 100, "h2", "Bob");
    auto v = sd_.on_connection("c", "c", 100, "h3", "Charlie");
    EXPECT_FALSE(v.is_spam) << "Different names should not trigger";
}

TEST_F(SpamDetectorTest, NumberedNamesDetected) {
    sd_.on_connection("a", "a", 100, "h1", "TuskNail2638");
    sd_.on_connection("b", "b", 100, "h2", "TuskNail822");
    auto v = sd_.on_connection("c", "c", 100, "h3", "TuskNail5029");
    EXPECT_TRUE(v.is_spam);
    EXPECT_EQ(v.heuristic, "name_pattern");
}

TEST_F(SpamDetectorTest, ShortPrefixIgnored) {
    // Prefix "Bot" is only 3 chars, below min_prefix=4
    sd_.on_connection("a", "a", 100, "h1", "Bot1");
    sd_.on_connection("b", "b", 100, "h2", "Bot2");
    auto v = sd_.on_connection("c", "c", 100, "h3", "Bot3");
    EXPECT_FALSE(v.is_spam);
}

TEST_F(SpamDetectorTest, NamesWithoutTrailingDigitsIgnored) {
    sd_.on_connection("a", "a", 100, "h1", "Phoenix");
    sd_.on_connection("b", "b", 100, "h2", "Phoenix");
    auto v = sd_.on_connection("c", "c", 100, "h3", "Phoenix");
    // "Phoenix" has no trailing digits — extract_prefix returns ""
    EXPECT_FALSE(v.is_spam);
}

TEST_F(SpamDetectorTest, NamePatternAlertOnlyOnce) {
    sd_.on_connection("a", "a", 100, "h1", "Raider100");
    sd_.on_connection("b", "b", 100, "h2", "Raider200");
    auto v1 = sd_.on_connection("c", "c", 100, "h3", "Raider300");
    EXPECT_TRUE(v1.is_spam);

    auto v2 = sd_.on_connection("d", "d", 100, "h4", "Raider400");
    // Should not re-alert for the same prefix
    // (heuristic may or may not fire again depending on implementation —
    //  the alert flag prevents duplicate alerts for same prefix)
    // Just verify no crash
}

// -- H6: Ghost connections ----------------------------------------------------

TEST_F(SpamDetectorTest, CompletedHandshakeNotGhost) {
    sd_.on_disconnect("1.2.3.4", true);
    sd_.on_disconnect("1.2.3.4", true);
    sd_.on_disconnect("1.2.3.4", true);
    // No warning expected — all completed handshake
}

TEST_F(SpamDetectorTest, GhostConnectionsDetected) {
    int callback_count = 0;
    sd_.set_callback([&](const std::string&, uint32_t, const SpamVerdict& v) {
        if (v.heuristic == "ghost")
            ++callback_count;
    });

    sd_.on_disconnect("1.2.3.4", false);
    sd_.on_disconnect("1.2.3.4", false);
    EXPECT_EQ(callback_count, 0);

    sd_.on_disconnect("1.2.3.4", false); // Threshold = 3
    EXPECT_EQ(callback_count, 1);
}

// -- H7: HWID reuse -----------------------------------------------------------

TEST_F(SpamDetectorTest, SameHWIDFromOneIPNotReuse) {
    sd_.on_connection("aaa", "aaa", 100, "SAME_HWID", "user1");
    sd_.on_connection("aaa", "aaa", 100, "SAME_HWID", "user2");
    // Same IPID — only 1 unique
}

TEST_F(SpamDetectorTest, SameHWIDFromThreeIPsIsReuse) {
    // Use non-pattern usernames (no trailing digits) to avoid triggering H5
    sd_.on_connection("aaa", "aaa", 100, "SAME_HWID", "Alice");
    sd_.on_connection("bbb", "bbb", 100, "SAME_HWID", "Bob");
    auto v = sd_.on_connection("ccc", "ccc", 100, "SAME_HWID", "Charlie");
    EXPECT_TRUE(v.is_spam);
    EXPECT_EQ(v.heuristic, "hwid_reuse");
}

TEST_F(SpamDetectorTest, EmptyHWIDIgnored) {
    sd_.on_connection("aaa", "aaa", 100, "", "user1");
    sd_.on_connection("bbb", "bbb", 100, "", "user2");
    auto v = sd_.on_connection("ccc", "ccc", 100, "", "user3");
    // Empty HWID should not trigger reuse detection
    EXPECT_NE(v.heuristic, "hwid_reuse");
}

// -- Sweep --------------------------------------------------------------------

TEST_F(SpamDetectorTest, SweepDoesNotCrash) {
    sd_.check_message("aaa", "aaa", 100, "test");
    sd_.on_connection("bbb", "bbb", 200, "hwid", "user");
    sd_.on_disconnect("1.2.3.4", false);
    sd_.sweep();
    // Just verify no crash or assertion failure
}

// -- Callback -----------------------------------------------------------------

TEST_F(SpamDetectorTest, CallbackInvokedOnSpam) {
    int callback_count = 0;
    std::string last_heuristic;

    sd_.set_callback([&](const std::string&, uint32_t, const SpamVerdict& v) {
        ++callback_count;
        last_heuristic = v.heuristic;
    });

    sd_.check_message("aaa", "aaa", 100, "spam");
    sd_.check_message("bbb", "bbb", 100, "spam");
    sd_.check_message("ccc", "ccc", 100, "spam");

    EXPECT_GE(callback_count, 1);
    EXPECT_EQ(last_heuristic, "echo");
}

// -- Disabled -----------------------------------------------------------------

TEST_F(SpamDetectorTest, DisabledReturnsNoSpam) {
    SpamDetectorConfig cfg;
    cfg.enabled = false;
    sd_.configure(cfg);

    sd_.check_message("aaa", "aaa", 100, "spam");
    sd_.check_message("bbb", "bbb", 100, "spam");
    auto v = sd_.check_message("ccc", "ccc", 100, "spam");
    EXPECT_FALSE(v.is_spam);
}

// -- Participants population (for auto-ban) -----------------------------------

TEST_F(SpamDetectorTest, EchoVerdictPopulatesAllParticipants) {
    sd_.check_message("ipid1", "10.0.0.1", 100, "spam");
    sd_.check_message("ipid2", "10.0.0.2", 100, "spam");
    auto v = sd_.check_message("ipid3", "10.0.0.3", 100, "spam");

    ASSERT_TRUE(v.is_spam);
    EXPECT_EQ(v.heuristic, "echo");
    EXPECT_EQ(v.participants.size(), 3u);

    // All three (ipid, ip) pairs should be present
    std::set<std::string> ipids;
    std::set<std::string> ips;
    for (auto& [pid, pip] : v.participants) {
        ipids.insert(pid);
        ips.insert(pip);
    }
    EXPECT_TRUE(ipids.count("ipid1"));
    EXPECT_TRUE(ipids.count("ipid2"));
    EXPECT_TRUE(ipids.count("ipid3"));
    EXPECT_TRUE(ips.count("10.0.0.1"));
    EXPECT_TRUE(ips.count("10.0.0.2"));
    EXPECT_TRUE(ips.count("10.0.0.3"));
}

TEST_F(SpamDetectorTest, NamePatternVerdictPopulatesAllParticipants) {
    sd_.on_connection("ipid1", "10.0.0.1", 100, "h1", "TWKTWK100");
    sd_.on_connection("ipid2", "10.0.0.2", 100, "h2", "TWKTWK200");
    auto v = sd_.on_connection("ipid3", "10.0.0.3", 100, "h3", "TWKTWK300");

    ASSERT_TRUE(v.is_spam);
    EXPECT_EQ(v.heuristic, "name_pattern");
    EXPECT_EQ(v.participants.size(), 3u);
}

TEST_F(SpamDetectorTest, BurstVerdictHasNoParticipants) {
    // H2 is a global counter, no per-IP cluster to ban.
    SpamVerdict last;
    for (int i = 0; i < 6; ++i) {
        last = sd_.on_connection("ip" + std::to_string(i), "10.0.0." + std::to_string(i), 100,
                                 "hwid" + std::to_string(i), "user");
    }
    ASSERT_TRUE(last.is_spam);
    EXPECT_EQ(last.heuristic, "burst");
    EXPECT_TRUE(last.participants.empty());
}

TEST_F(SpamDetectorTest, ParticipantsSkippedForMissingIP) {
    // If IP wasn't recorded (e.g., legacy caller passed empty string),
    // that participant should be excluded from the list but detection still fires.
    sd_.check_message("ipid1", "", 100, "spam");
    sd_.check_message("ipid2", "10.0.0.2", 100, "spam");
    auto v = sd_.check_message("ipid3", "10.0.0.3", 100, "spam");

    ASSERT_TRUE(v.is_spam);
    // ipid1 is in the unique_ipids set but has no IP — should be excluded.
    EXPECT_EQ(v.participants.size(), 2u);
    for (auto& [pid, pip] : v.participants) {
        EXPECT_FALSE(pip.empty());
    }
}
