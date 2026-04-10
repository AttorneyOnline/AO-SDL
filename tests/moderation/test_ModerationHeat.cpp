#include "moderation/ContentModerationConfig.h"
#include "moderation/ModerationHeat.h"
#include "moderation/ModerationTypes.h"

#include <gtest/gtest.h>

namespace {

using moderation::HeatConfig;
using moderation::ModerationAction;
using moderation::ModerationHeat;

// A test clock lets us drive decay math deterministically without
// sleeping, which is both flaky and slow.
int64_t g_fake_clock_ms = 0;
int64_t fake_clock() {
    return g_fake_clock_ms;
}

HeatConfig test_config() {
    HeatConfig c;
    c.decay_half_life_seconds = 60.0; // short for tests
    c.censor_threshold = 1.0;
    c.drop_threshold = 3.0;
    c.mute_threshold = 6.0;
    c.kick_threshold = 10.0;
    c.ban_threshold = 15.0;
    c.perma_ban_threshold = 25.0;
    c.prune_below = 0.01;
    c.sweep_idle_seconds = 1;
    return c;
}

class ModerationHeatTest : public ::testing::Test {
  protected:
    void SetUp() override {
        g_fake_clock_ms = 1'000'000'000'000LL;
        heat_.set_clock_for_tests(fake_clock);
        heat_.configure(test_config());
    }

    void advance_seconds(double s) {
        g_fake_clock_ms += static_cast<int64_t>(s * 1000.0);
    }

    ModerationHeat heat_;
};

} // namespace

TEST_F(ModerationHeatTest, FirstApplyReturnsRaw) {
    EXPECT_DOUBLE_EQ(heat_.apply("a", 2.0), 2.0);
}

TEST_F(ModerationHeatTest, AccumulatesWithoutElapsedTime) {
    heat_.apply("a", 1.0);
    EXPECT_DOUBLE_EQ(heat_.apply("a", 2.0), 3.0);
}

TEST_F(ModerationHeatTest, DecaysByHalfOverHalfLife) {
    heat_.apply("a", 4.0);
    // Half-life = 60s.
    advance_seconds(60.0);
    double v = heat_.peek("a");
    EXPECT_NEAR(v, 2.0, 0.01);
}

TEST_F(ModerationHeatTest, DecaysContinuouslyNotStepwise) {
    heat_.apply("a", 8.0);
    advance_seconds(30.0); // one quarter of half-life... wait no, 30s is half of 60s = 1 half-life? No, half-life IS 60s.
    // 30 / 60 = 0.5 half-lives → factor = 2^-0.5 ≈ 0.707
    double v = heat_.peek("a");
    EXPECT_NEAR(v, 8.0 * 0.7071, 0.05);
}

TEST_F(ModerationHeatTest, DecideMapsBelowThresholdToLog) {
    EXPECT_EQ(heat_.decide(0.5), ModerationAction::LOG);
}

TEST_F(ModerationHeatTest, DecideMapsZeroToNone) {
    EXPECT_EQ(heat_.decide(0.0), ModerationAction::NONE);
}

TEST_F(ModerationHeatTest, DecideLadderCorrect) {
    EXPECT_EQ(heat_.decide(1.5), ModerationAction::CENSOR);
    EXPECT_EQ(heat_.decide(4.0), ModerationAction::DROP);
    EXPECT_EQ(heat_.decide(7.0), ModerationAction::MUTE);
    EXPECT_EQ(heat_.decide(12.0), ModerationAction::KICK);
    EXPECT_EQ(heat_.decide(20.0), ModerationAction::BAN);
    EXPECT_EQ(heat_.decide(100.0), ModerationAction::PERMA_BAN);
}

TEST_F(ModerationHeatTest, ResetClearsHeat) {
    heat_.apply("a", 5.0);
    heat_.reset("a");
    EXPECT_DOUBLE_EQ(heat_.peek("a"), 0.0);
}

TEST_F(ModerationHeatTest, SweepPrunesBelowThresholdAndIdle) {
    heat_.apply("a", 10.0);
    heat_.apply("b", 0.005);

    // Advance past sweep_idle (1s) so 'b' becomes prunable.
    advance_seconds(2.0);
    heat_.sweep();

    // 'a' still has ~9.77 heat, stays.
    EXPECT_GT(heat_.peek("a"), 1.0);
    // 'b' was below prune_below and idle → pruned.
    EXPECT_EQ(heat_.peek("b"), 0.0);
}

TEST_F(ModerationHeatTest, ApplyNegativeTreatedAsZero) {
    heat_.apply("a", 5.0);
    heat_.apply("a", -1.0); // should not decrement
    EXPECT_DOUBLE_EQ(heat_.peek("a"), 5.0);
}

TEST_F(ModerationHeatTest, ConcurrentUsersTrackedIndependently) {
    heat_.apply("alice", 5.0);
    heat_.apply("bob", 2.0);
    EXPECT_DOUBLE_EQ(heat_.peek("alice"), 5.0);
    EXPECT_DOUBLE_EQ(heat_.peek("bob"), 2.0);
}
