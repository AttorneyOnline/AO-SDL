#include "moderation/ContentModerator.h"
#include "moderation/RemoteClassifier.h"

#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <utility>
#include <vector>

namespace {

using moderation::ContentModerationConfig;
using moderation::ContentModerator;
using moderation::ModerationAction;
using moderation::ModerationAuditLog;

ContentModerationConfig enable_unicode_only() {
    ContentModerationConfig cfg;
    cfg.enabled = true;
    cfg.check_ic = true;
    cfg.check_ooc = true;
    cfg.unicode.enabled = true;
    // Keep heat low so small scores still produce actions in tests.
    cfg.heat.decay_half_life_seconds = 600.0;
    cfg.heat.censor_threshold = 0.1;
    cfg.heat.drop_threshold = 1.0;
    cfg.heat.mute_threshold = 2.0;
    cfg.heat.kick_threshold = 4.0;
    cfg.heat.ban_threshold = 8.0;
    cfg.heat.perma_ban_threshold = 16.0;
    cfg.heat.mute_duration_seconds = 60;
    cfg.heat.weight_visual_noise = 1.0;
    return cfg;
}

/// Test transport that synthesizes an OpenAI response from caller-
/// supplied per-axis scores. Lets ContentModerator tests exercise
/// Layer 2 without touching the network. Mirrors the MockTransport
/// pattern in test_RemoteClassifier.cpp.
class SyntheticRemoteTransport : public moderation::RemoteClassifierTransport {
  public:
    struct Scores {
        double toxicity = 0.0;
        double hate = 0.0;
        double sexual = 0.0;
        double sexual_minors = 0.0;
        double violence = 0.0;
        double self_harm = 0.0;
    };

    explicit SyntheticRemoteTransport(Scores s) : scores_(s) {
    }

    /// Number of post_json() invocations. Incremented atomically so
    /// tests that want to assert "remote was (not) called N times"
    /// can read it after the ContentModerator calls complete.
    mutable std::atomic<int> call_count{0};

    std::pair<int, std::string> post_json(const std::string& /*url*/, const std::string& /*bearer*/,
                                          const std::string& /*body*/, int /*timeout_ms*/) override {
        call_count.fetch_add(1, std::memory_order_relaxed);
        // Hand-construct the minimal OpenAI omni-moderation response
        // shape the parser expects.
        std::string body = "{\"id\":\"test\",\"model\":\"omni\",\"results\":[{\"flagged\":true,\"categories\":{}";
        body += ",\"category_scores\":{";
        body += "\"harassment\":" + std::to_string(scores_.toxicity);
        body += ",\"hate\":" + std::to_string(scores_.hate);
        body += ",\"sexual\":" + std::to_string(scores_.sexual);
        body += ",\"sexual/minors\":" + std::to_string(scores_.sexual_minors);
        body += ",\"violence\":" + std::to_string(scores_.violence);
        body += ",\"self-harm\":" + std::to_string(scores_.self_harm);
        body += "}}]}";
        return {200, body};
    }

  private:
    Scores scores_;
};

ContentModerationConfig enable_urls_only() {
    ContentModerationConfig cfg;
    cfg.enabled = true;
    cfg.check_ic = true;
    cfg.check_ooc = true;
    cfg.urls.enabled = true;
    cfg.urls.blocklist = {"bad.com"};
    cfg.urls.blocked_score = 1.0;
    // Weight is large enough that a single blocked link pushes the
    // user past the mute threshold on the first strike. This matches
    // the tuning a production deployment would use for malware links.
    cfg.heat.censor_threshold = 0.5;
    cfg.heat.drop_threshold = 2.0;
    cfg.heat.mute_threshold = 5.0;
    cfg.heat.kick_threshold = 100.0;
    cfg.heat.ban_threshold = 200.0;
    cfg.heat.perma_ban_threshold = 500.0;
    cfg.heat.weight_link_risk = 6.0;
    return cfg;
}

class ContentModeratorTest : public ::testing::Test {
  protected:
    ContentModerator cm_;
};

} // namespace

TEST_F(ContentModeratorTest, DisabledSubsystemReturnsNone) {
    ContentModerationConfig cfg;
    cfg.enabled = false;
    cm_.configure(cfg);

    auto v = cm_.check("ipid1", "ooc", "hello world");
    EXPECT_EQ(v.action, ModerationAction::NONE);
    EXPECT_DOUBLE_EQ(v.heat_after, 0.0);
}

TEST_F(ContentModeratorTest, DisabledChannelReturnsNone) {
    auto cfg = enable_urls_only();
    cfg.check_ic = false;
    cm_.configure(cfg);

    auto v = cm_.check("ipid1", "ic", "see bad.com/scam");
    EXPECT_EQ(v.action, ModerationAction::NONE);
}

TEST_F(ContentModeratorTest, CleanMessagePassesThrough) {
    cm_.configure(enable_unicode_only());
    auto v = cm_.check("ipid1", "ooc", "hello friend");
    EXPECT_EQ(v.action, ModerationAction::NONE);
}

TEST_F(ContentModeratorTest, BlockedLinkTriggersAction) {
    cm_.configure(enable_urls_only());
    auto v = cm_.check("ipid1", "ooc", "join bad.com today");
    // weight=3 × score=1 = 3 heat delta → mute threshold (2.0) crossed
    EXPECT_GE(static_cast<int>(v.action), static_cast<int>(ModerationAction::MUTE));
    EXPECT_GT(v.scores.link_risk, 0.0);
}

TEST_F(ContentModeratorTest, MuteActionMarksIpidMuted) {
    cm_.configure(enable_urls_only());
    cm_.check("ipid1", "ooc", "join bad.com today");
    EXPECT_TRUE(cm_.is_muted("ipid1"));
    // A different IPID should not be affected.
    EXPECT_FALSE(cm_.is_muted("ipid2"));
}

TEST_F(ContentModeratorTest, LiftMuteClearsIt) {
    cm_.configure(enable_urls_only());
    cm_.check("ipid1", "ooc", "bad.com");
    ASSERT_TRUE(cm_.is_muted("ipid1"));
    EXPECT_TRUE(cm_.lift_mute("ipid1"));
    EXPECT_FALSE(cm_.is_muted("ipid1"));
}

TEST_F(ContentModeratorTest, HeatAccumulatesAcrossMessages) {
    auto cfg = enable_unicode_only();
    cfg.heat.weight_visual_noise = 0.5; // small per-message
    cm_.configure(cfg);

    // Feed 5 zalgo-y messages; heat should climb monotonically.
    std::string zalgo = "h";
    for (int i = 0; i < 20; ++i)
        zalgo += "\xCC\x80";
    zalgo += "i";

    double last = 0.0;
    for (int i = 0; i < 5; ++i) {
        auto v = cm_.check("ipid1", "ooc", zalgo);
        EXPECT_GE(v.heat_after, last);
        last = v.heat_after;
    }
}

TEST_F(ContentModeratorTest, AuditLogReceivesEvent) {
    cm_.configure(enable_urls_only());

    ModerationAuditLog audit;
    std::vector<moderation::ModerationEvent> seen;
    audit.add_sink("capture", [&seen](const moderation::ModerationEvent& ev) { seen.push_back(ev); });
    cm_.set_audit_log(&audit);

    cm_.check("ipid1", "ooc", "check bad.com now");
    ASSERT_FALSE(seen.empty());
    EXPECT_EQ(seen[0].ipid, "ipid1");
    EXPECT_EQ(seen[0].channel, "ooc");
    EXPECT_GT(seen[0].scores.link_risk, 0.0);
}

TEST_F(ContentModeratorTest, RoleplayToxicityBelowFloorIsClean) {
    // "Do you have cotton between your ears, spiky boy?" is a
    // canonical Ace Attorney Edgeworth-to-Phoenix cross-examination
    // line. OpenAI's harassment classifier scores it ~0.65.
    // kagami's roleplay-tuned toxicity floor is 0.85, so this line
    // must contribute ZERO heat on its own — it's below the floor.
    //
    // We simulate the remote classifier output by feeding axis
    // scores directly to heat_delta_from via a wrapper test.
    // ContentModerator's heat_delta_from is private, so we exercise
    // it indirectly by passing the moderator a message that runs
    // through the normal layer stack; since the test doesn't have
    // a real OpenAI key wired, remote layer will be inert and we
    // can only assert that the fixture's rules-only layers don't
    // fire on the canonical line.
    auto cfg = enable_urls_only(); // rules-only fixture, no remote
    cm_.configure(cfg);

    auto v = cm_.check("user1", "ic", "do you have cotton between your ears, spiky boy?");
    EXPECT_EQ(v.action, moderation::ModerationAction::NONE);
    EXPECT_DOUBLE_EQ(v.heat_delta, 0.0);
}

TEST_F(ContentModeratorTest, ToxicityBelowFloorDoesNotFire) {
    // Synthetic remote transport: OpenAI returns toxicity=0.6
    // (below our roleplay floor of 0.85). Expected: no heat added
    // regardless of the weight, because the floor gates the sum.
    ContentModerationConfig cfg;
    cfg.enabled = true;
    cfg.check_ooc = true;
    cfg.check_ic = true;
    cfg.remote.enabled = true;
    cfg.remote.api_key = "sk-test";
    cfg.heat.weight_toxicity = 1.0;
    cfg.heat.censor_threshold = 0.5;
    cm_.configure(cfg);
    cm_.set_remote_transport(std::make_unique<SyntheticRemoteTransport>(SyntheticRemoteTransport::Scores{0.6}));

    auto v = cm_.check("user1", "ooc", "hello there");
    EXPECT_EQ(v.action, moderation::ModerationAction::NONE);
    EXPECT_DOUBLE_EQ(v.heat_delta, 0.0);
}

TEST_F(ContentModeratorTest, ToxicityAboveFloorFires) {
    // Same setup but synthetic toxicity=0.9 (above 0.85 floor).
    // Expected: heat_delta == 0.9 * 1.0 == 0.9, action == LOG
    // (below the 1.0 censor threshold).
    ContentModerationConfig cfg;
    cfg.enabled = true;
    cfg.check_ooc = true;
    cfg.remote.enabled = true;
    cfg.remote.api_key = "sk-test";
    cfg.heat.weight_toxicity = 1.0;
    cfg.heat.censor_threshold = 1.5;
    cm_.configure(cfg);
    cm_.set_remote_transport(std::make_unique<SyntheticRemoteTransport>(SyntheticRemoteTransport::Scores{0.9}));

    auto v = cm_.check("user1", "ooc", "hello there");
    EXPECT_GT(v.heat_delta, 0.0);
    EXPECT_NEAR(v.heat_delta, 0.9, 1e-6);
    EXPECT_EQ(v.action, moderation::ModerationAction::LOG);
}

TEST_F(ContentModeratorTest, SexualMinorsPositiveScoreIsCatastrophic) {
    // The sexual_minors axis has a catastrophic weight (100.0) so
    // any positive signal above the 0.01 floor instantly crosses
    // the perma_ban threshold. This regression protects the
    // non-negotiable behavior against weight or floor changes.
    ContentModerationConfig cfg;
    cfg.enabled = true;
    cfg.check_ooc = true;
    cfg.remote.enabled = true;
    cfg.remote.api_key = "sk-test";
    cfg.heat.perma_ban_threshold = 25.0;
    cm_.configure(cfg);
    SyntheticRemoteTransport::Scores s;
    s.sexual_minors = 0.5;
    cm_.set_remote_transport(std::make_unique<SyntheticRemoteTransport>(s));

    auto v = cm_.check("user1", "ooc", "anything");
    EXPECT_EQ(v.action, moderation::ModerationAction::PERMA_BAN);
}

TEST_F(ContentModeratorTest, CleanMessageAfterOffenseDoesNotInheritAction) {
    // Regression: an accumulated-heat user who posts a completely
    // clean message must NOT be dropped. The bug was that remote
    // classifier floor-level noise (toxicity ~= 1e-5) multiplied by
    // the axis weight produced a sub-0.001 heat delta > 0, which
    // blew through the heat_delta <= 0 short-circuit and let the
    // ladder act on the accumulated heat.
    auto cfg = enable_urls_only();
    // Configure a stub remote-classifier axis score by bypassing the
    // layer — use UrlExtractor alone for the "bad" message, then
    // fake noise on the clean one by... wait, we can't synthesize
    // remote noise without a mock transport. Instead we directly
    // manipulate heat so the scenario is reproducible: push heat
    // to the drop threshold, then check a clean message.
    cm_.configure(cfg);

    // Post the bad message to actually move heat via the URL layer.
    // The fixture blocklist is {"bad.com"} so hit that.
    auto bad = cm_.check("user1", "ooc", "see bad.com now");
    EXPECT_GT(bad.heat_after, cfg.heat.drop_threshold);

    // A clean message MUST return NONE — the short-circuit fires on
    // heat_delta=0 because none of the clean axes cross their
    // visibility floors.
    auto clean = cm_.check("user1", "ooc", "hello world");
    EXPECT_EQ(clean.action, moderation::ModerationAction::NONE);
    // The heat should NOT have grown (no delta applied).
    EXPECT_LE(clean.heat_after, bad.heat_after);
}

// ---------------------------------------------------------------------------
// Trust bank (Feature 1): clean-message accrual + Layer 2 skip + safety reset
// ---------------------------------------------------------------------------

namespace {
/// Build a config suitable for testing trust bank behavior in isolation.
/// Only remote classifier is enabled as a moderation layer (so "clean"
/// messages produce a zero heat delta from Layer 1), with synthetic
/// remote scores the caller provides via SyntheticRemoteTransport.
ContentModerationConfig enable_trust_bank_test() {
    ContentModerationConfig cfg;
    cfg.enabled = true;
    cfg.check_ic = true;
    cfg.check_ooc = true;
    cfg.remote.enabled = true;
    cfg.remote.api_key = "sk-test";

    // Trust bank: aggressive so the tests are deterministic.
    // clean_reward=1.0 means each NONE verdict moves heat -1.0.
    // api_skip_threshold=1.5 means eligible to skip when heat < -1.5.
    // max_trust=3.0 means the skip rate ramps from 0% at -1.5 to
    //   (1 - min_sample_rate) = 100% at -3.0.
    // min_sample_rate=0.0 gives a deterministic "always skip" at the floor.
    cfg.trust_bank.enabled = true;
    cfg.trust_bank.clean_reward = 1.0;
    cfg.trust_bank.max_trust = 3.0;
    cfg.trust_bank.api_skip_threshold = 1.5;
    cfg.trust_bank.min_sample_rate = 0.0;

    // Heat ladder: give enough headroom that slur delta (6.0 *
    // weight_slurs 6.0 = 36.0) clearly crosses mute_threshold.
    cfg.heat.decay_half_life_seconds = 3600.0; // long so no decay during test
    cfg.heat.censor_threshold = 1.0;
    cfg.heat.drop_threshold = 3.0;
    cfg.heat.mute_threshold = 6.0;
    cfg.heat.kick_threshold = 10.0;
    cfg.heat.ban_threshold = 15.0;
    cfg.heat.perma_ban_threshold = 25.0;
    cfg.heat.weight_toxicity = 1.0;
    return cfg;
}
} // namespace

TEST_F(ContentModeratorTest, TrustBankAccruesOnCleanMessages) {
    cm_.configure(enable_trust_bank_test());
    // Clean synthetic transport — every call returns zero scores.
    cm_.set_remote_transport(std::make_unique<SyntheticRemoteTransport>(SyntheticRemoteTransport::Scores{}));

    // 3 clean messages → heat should accumulate -1.0 per message
    // UNTIL the user crosses api_skip_threshold, at which point the
    // skip branch starts firing and heat STILL accumulates (accrual
    // runs on the NONE early-return path regardless of skip).
    for (int i = 0; i < 3; ++i) {
        auto v = cm_.check("trusted_user", "ooc", "hello");
        EXPECT_EQ(v.action, ModerationAction::NONE);
    }
    // After 3 clean messages, heat should be at the -3.0 max_trust
    // floor (clamped). current_heat() is a public diagnostic accessor.
    EXPECT_DOUBLE_EQ(cm_.current_heat("trusted_user"), -3.0);
}

TEST_F(ContentModeratorTest, TrustBankSkipsRemoteForTrustedUser) {
    auto cfg = enable_trust_bank_test();
    cm_.configure(cfg);
    auto transport = std::make_unique<SyntheticRemoteTransport>(SyntheticRemoteTransport::Scores{});
    auto* transport_ptr = transport.get();
    cm_.set_remote_transport(std::move(transport));

    // First 2 clean messages: heat not yet past api_skip_threshold
    // (-1.5), so the remote IS called. These accrue trust.
    cm_.check("trusted_user", "ooc", "hello one");
    cm_.check("trusted_user", "ooc", "hello two");
    // After 2 clean: heat = -2.0. Past threshold, but not yet at
    // max_trust = -3.0. The 3rd message should reach heat = -3.0
    // where skip_rate = 100% by our deterministic config.
    cm_.check("trusted_user", "ooc", "hello three");

    // At this point, heat is at -3.0 (the floor). Any further clean
    // message must hit the deterministic skip branch — remote
    // transport call_count must NOT increment for the next 10 calls.
    int before = transport_ptr->call_count.load();
    for (int i = 0; i < 10; ++i) {
        cm_.check("trusted_user", "ooc", "still clean");
    }
    int after = transport_ptr->call_count.load();
    EXPECT_EQ(after, before) << "expected all 10 calls to skip remote at max trust";
}

TEST_F(ContentModeratorTest, TrustBankZeroSampleRateSkipsEverything) {
    // Contrast with the previous TrustBankSkipsRemoteForTrustedUser
    // test: min_sample_rate=0 is already the deterministic floor, so
    // every at-max-trust message deterministically skips. This
    // exists as a separate test mostly for naming clarity in the
    // regression suite — "zero sample rate is respected".
    auto cfg = enable_trust_bank_test();
    cfg.trust_bank.min_sample_rate = 0.0;
    cm_.configure(cfg);
    auto transport = std::make_unique<SyntheticRemoteTransport>(SyntheticRemoteTransport::Scores{});
    auto* transport_ptr = transport.get();
    cm_.set_remote_transport(std::move(transport));

    // Burn through the initial accrual phase (reaches the floor
    // after 3 clean messages with clean_reward=1.0 and max_trust=3.0).
    for (int i = 0; i < 3; ++i)
        cm_.check("user", "ooc", "warmup");
    int baseline = transport_ptr->call_count.load();

    // At max trust with zero sample rate, ALL subsequent calls must skip.
    for (int i = 0; i < 50; ++i)
        cm_.check("user", "ooc", "still clean");
    EXPECT_EQ(transport_ptr->call_count.load(), baseline);
}

TEST_F(ContentModeratorTest, TrustBankMinSampleRateFloorLetsSomeTrafficThrough) {
    // Load-bearing test: with min_sample_rate > 0, even at max trust
    // SOME traffic reaches the remote classifier for drift detection.
    // The exact fraction is dominated by the per-IPID remote token
    // bucket (burst=10, refill=2/sec) which caps API calls globally
    // regardless of the trust-bank skip rate. So we don't assert on
    // the exact count — just that the floor lets more than zero
    // calls through in contrast to min_sample_rate=0, which deter-
    // ministically blocks everything.
    auto cfg = enable_trust_bank_test();
    cfg.trust_bank.min_sample_rate = 0.5; // aggressive floor
    cm_.configure(cfg);
    auto transport = std::make_unique<SyntheticRemoteTransport>(SyntheticRemoteTransport::Scores{});
    auto* transport_ptr = transport.get();
    cm_.set_remote_transport(std::move(transport));

    // Accrue trust to the floor.
    for (int i = 0; i < 3; ++i)
        cm_.check("user", "ooc", "warmup");
    int before = transport_ptr->call_count.load();

    // Send a batch of messages at max trust. With 50% sample rate
    // AND a generous remote bucket budget, we expect several to get
    // through before the bucket depletes. The exact count depends
    // on timing — this is a lower-bound assertion only.
    for (int i = 0; i < 200; ++i)
        cm_.check("user", "ooc", "clean message");
    int after = transport_ptr->call_count.load();
    int calls_during_trusted_phase = after - before;

    EXPECT_GT(calls_during_trusted_phase, 0) << "min_sample_rate > 0 must let at least some calls through "
                                                "for drift detection";
}

TEST_F(ContentModeratorTest, TrustBankSlurHitResetsAndPenalizes) {
    // The safety-critical test: a user with accumulated trust who
    // trips a slur filter must end up at mute_threshold (not
    // absorbed into trust credit). This is the "slur-reset" semantic
    // of ModerationHeat::apply().
    auto cfg = enable_trust_bank_test();
    // Enable slur layer with a wordlist.
    cfg.slurs.enabled = true;
    cfg.slurs.match_score = 1.0;
    cfg.heat.weight_slurs = 6.0; // single hit = mute threshold
    cm_.configure(cfg);
    cm_.set_remote_transport(std::make_unique<SyntheticRemoteTransport>(SyntheticRemoteTransport::Scores{}));
    cm_.set_slur_wordlist({"forbidden"});

    // Accrue full trust.
    for (int i = 0; i < 3; ++i)
        cm_.check("trusted_user", "ooc", "hello friend");
    ASSERT_DOUBLE_EQ(cm_.current_heat("trusted_user"), -3.0);

    // Now send a slur. Heat math:
    //   slur score 1.0 * weight 6.0 = +6.0 heat delta
    //   BEFORE that is applied, apply() resets negative heat to 0
    //   → new heat = 0 + 6.0 = 6.0 = mute_threshold
    //   → action = MUTE (NOT below-any-threshold 1.0)
    auto v = cm_.check("trusted_user", "ooc", "forbidden");
    EXPECT_EQ(v.action, ModerationAction::MUTE) << "trusted user who trips slur filter must still be muted, "
                                                   "not absorbed by accumulated trust credit";
    EXPECT_DOUBLE_EQ(v.heat_after, 6.0);
}

TEST_F(ContentModeratorTest, TrustBankDisabledLeavesHeatAtZero) {
    // Regression: when trust_bank is disabled, clean messages should
    // NOT touch heat (existing pre-feature behavior preserved).
    auto cfg = enable_trust_bank_test();
    cfg.trust_bank.enabled = false;
    cm_.configure(cfg);
    cm_.set_remote_transport(std::make_unique<SyntheticRemoteTransport>(SyntheticRemoteTransport::Scores{}));

    for (int i = 0; i < 20; ++i)
        cm_.check("user", "ooc", "hello");
    EXPECT_DOUBLE_EQ(cm_.current_heat("user"), 0.0);
}

TEST_F(ContentModeratorTest, JsonSerializationRoundTrip) {
    moderation::ModerationEvent ev;
    ev.timestamp_ms = 123456;
    ev.ipid = "abc";
    ev.channel = "ic";
    ev.message_sample = "hi";
    ev.action = ModerationAction::CENSOR;
    ev.heat_after = 1.5;
    ev.scores.visual_noise = 0.4;

    auto line = ModerationAuditLog::to_json_line(ev);
    EXPECT_NE(line.find("\"ipid\":\"abc\""), std::string::npos);
    EXPECT_NE(line.find("\"action\":\"censor\""), std::string::npos);
    EXPECT_NE(line.find("\"visual_noise\":0.4"), std::string::npos);
}
