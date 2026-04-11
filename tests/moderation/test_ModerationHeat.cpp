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
    advance_seconds(
        30.0); // one quarter of half-life... wait no, 30s is half of 60s = 1 half-life? No, half-life IS 60s.
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

// ---------------------------------------------------------------------------
// Trust bank (negative heat) tests
// ---------------------------------------------------------------------------

TEST_F(ModerationHeatTest, AccrueTrustSubtractsFromZero) {
    double v = heat_.accrue_trust("a", 0.1, -5.0);
    EXPECT_DOUBLE_EQ(v, -0.1);
    EXPECT_DOUBLE_EQ(heat_.peek("a"), -0.1);
}

TEST_F(ModerationHeatTest, AccrueTrustRespectsFloor) {
    // Call accrue_trust 100 times with reward=0.1 and floor=-5.0.
    // Expected final heat is exactly -5.0 (the floor), NOT -10.0
    // even though raw math would give -10.0.
    for (int i = 0; i < 100; ++i)
        heat_.accrue_trust("a", 0.1, -5.0);
    EXPECT_DOUBLE_EQ(heat_.peek("a"), -5.0);
}

TEST_F(ModerationHeatTest, AccrueTrustNoOpWhenSuspicious) {
    // User in suspicion state (heat > 0) cannot build trust.
    heat_.apply("a", 3.0);
    double before = heat_.peek("a");
    double after = heat_.accrue_trust("a", 0.1, -5.0);
    EXPECT_DOUBLE_EQ(after, before);
    EXPECT_DOUBLE_EQ(heat_.peek("a"), before);
}

TEST_F(ModerationHeatTest, AccrueTrustRejectsPositiveFloor) {
    // Guard against caller mistakes — a non-negative floor would
    // make accrue_trust pointless or worse, push heat upward.
    heat_.accrue_trust("a", 0.1, 5.0);
    EXPECT_DOUBLE_EQ(heat_.peek("a"), 0.0);
}

TEST_F(ModerationHeatTest, AccrueTrustRejectsNonPositiveAmount) {
    heat_.accrue_trust("a", 0.0, -5.0);
    EXPECT_DOUBLE_EQ(heat_.peek("a"), 0.0);
    heat_.accrue_trust("a", -1.0, -5.0);
    EXPECT_DOUBLE_EQ(heat_.peek("a"), 0.0);
}

TEST_F(ModerationHeatTest, NegativeHeatDecaysTowardZero) {
    // Trust is subject to the same exponential decay as suspicion.
    // Set up -4.0 trust, advance one half-life, expect -2.0.
    for (int i = 0; i < 40; ++i)
        heat_.accrue_trust("a", 0.1, -5.0);
    ASSERT_DOUBLE_EQ(heat_.peek("a"), -4.0);
    advance_seconds(60.0);
    double v = heat_.peek("a");
    EXPECT_NEAR(v, -2.0, 0.01);
}

TEST_F(ModerationHeatTest, NegativeHeatSnapsToZeroAtFloor) {
    // The abs() fix in decay_locked should snap tiny negatives to
    // zero, not clobber meaningful ones. Set up a very small
    // negative value and verify it snaps to exactly 0.
    heat_.accrue_trust("a", 1e-7, -5.0); // well below the 1e-6 snap
    advance_seconds(1.0);                // trigger a decay pass
    EXPECT_DOUBLE_EQ(heat_.peek("a"), 0.0);
}

TEST_F(ModerationHeatTest, MeaningfulNegativeHeatSurvivesDecayPass) {
    // Confirm the mirror case: a value clearly above the 1e-6 snap
    // on the negative side MUST survive a decay pass.
    heat_.accrue_trust("a", 3.0, -5.0); // heat = -3.0
    advance_seconds(0.001);             // tiny advance — barely any decay
    double v = heat_.peek("a");
    EXPECT_LT(v, -1.0);  // still meaningfully negative
    EXPECT_GT(v, -3.01); // very close to the original -3.0
}

TEST_F(ModerationHeatTest, PositiveDeltaResetsNegativeHeat) {
    // The critical safety invariant of the trust bank: accumulated
    // trust credit cannot absorb a slur hit. A user with -5.0 trust
    // who sends a slur (delta=6.0) must end up at heat=6.0 (mute
    // threshold), NOT at 1.0 (below any threshold). The apply()
    // method resets negative heat to zero before adding delta.
    for (int i = 0; i < 50; ++i)
        heat_.accrue_trust("a", 0.1, -10.0);
    ASSERT_DOUBLE_EQ(heat_.peek("a"), -5.0);
    double new_heat = heat_.apply("a", 6.0);
    EXPECT_DOUBLE_EQ(new_heat, 6.0);
    EXPECT_EQ(heat_.decide(new_heat), ModerationAction::MUTE);
}

TEST_F(ModerationHeatTest, PartialTrustResetOnSmallPositiveDelta) {
    // Even a sub-censor delta (0.5) applied to a trusted user
    // should reset to zero first, landing the user at exactly 0.5
    // (LOG level). The trust is GONE — not merely reduced.
    for (int i = 0; i < 50; ++i)
        heat_.accrue_trust("a", 0.1, -10.0);
    ASSERT_DOUBLE_EQ(heat_.peek("a"), -5.0);
    double new_heat = heat_.apply("a", 0.5);
    EXPECT_DOUBLE_EQ(new_heat, 0.5);
    EXPECT_EQ(heat_.decide(new_heat), ModerationAction::LOG);
}

TEST_F(ModerationHeatTest, TrustAccrualIsTrackedPerUser) {
    heat_.accrue_trust("alice", 0.5, -5.0);
    heat_.accrue_trust("bob", 0.2, -5.0);
    EXPECT_DOUBLE_EQ(heat_.peek("alice"), -0.5);
    EXPECT_DOUBLE_EQ(heat_.peek("bob"), -0.2);
}

TEST_F(ModerationHeatTest, SweepPreservesMeaningfulNegativeHeat) {
    // Regression for a latent bug exposed by trust-bank: the sweep()
    // prune condition used `heat < prune_below` which is trivially
    // true for any negative value. Without the abs() fix, a trusted
    // user would lose all accumulated credit on the very first idle
    // sweep after accrual. Confirm a meaningfully-trusted user
    // survives a sweep.
    heat_.accrue_trust("alice", 5.0, -10.0);
    ASSERT_DOUBLE_EQ(heat_.peek("alice"), -5.0);
    advance_seconds(2.0); // past sweep_idle
    heat_.sweep();
    // Alice still has negative heat (decayed slightly from -5.0).
    double v = heat_.peek("alice");
    EXPECT_LT(v, -4.0);
    EXPECT_GT(v, -5.01);
}
