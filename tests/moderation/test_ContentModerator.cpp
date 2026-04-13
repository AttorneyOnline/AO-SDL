#include "moderation/ContentModerator.h"
#include "moderation/EmbeddingBackend.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
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

// SyntheticRemoteTransport removed — no remote classifier in v2.

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

TEST_F(ContentModeratorTest, RoleplayLineBelowFloorIsClean) {
    // "Do you have cotton between your ears, spiky boy?" is a
    // canonical Ace Attorney Edgeworth-to-Phoenix cross-examination
    // line. OpenAI's harassment classifier scores it ~0.65.
    // kagami's roleplay-tuned floors ensure this line contributes
    // ZERO heat on its own — it's below the threshold.
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

// Tests for remote-classifier-dependent behavior (SexualMinors,
// TrustBank*) were removed alongside the remote classifier in the
// MLP v2 branch.

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

// ===================================================================
// Shared helpers for new tests
// ===================================================================

namespace {

// Deterministic mock embedding backend: returns caller-controlled unit
// vectors via a lookup table. Falls back to a default vector for any
// unregistered text. Same pattern as test_BadHintLayer.cpp.
class MockBackend : public moderation::EmbeddingBackend {
  public:
    explicit MockBackend(int dim = 3) : dim_(dim) {
    }

    int dimension() const override {
        return dim_;
    }
    bool is_ready() const override {
        return true;
    }
    moderation::EmbeddingResult embed(std::string_view text) override {
        moderation::EmbeddingResult r;
        auto it = table_.find(std::string(text));
        if (it != table_.end()) {
            r.ok = true;
            r.vector = it->second;
            return r;
        }
        // Return the default vector if one is set.
        if (!default_vec_.empty()) {
            r.ok = true;
            r.vector = default_vec_;
            return r;
        }
        r.error = "no mock entry";
        return r;
    }
    const char* name() const override {
        return "mock";
    }

    void set(const std::string& text, std::vector<float> v) {
        if (static_cast<int>(v.size()) != dim_)
            dim_ = static_cast<int>(v.size());
        table_[text] = std::move(v);
    }

    // Set a default embedding returned for any text not in the table.
    void set_default(std::vector<float> v) {
        if (static_cast<int>(v.size()) != dim_)
            dim_ = static_cast<int>(v.size());
        default_vec_ = std::move(v);
    }

  private:
    int dim_;
    std::unordered_map<std::string, std::vector<float>> table_;
    std::vector<float> default_vec_;
};

// Build a v2 MLP weight blob in memory with caller-controlled weights.
//
// V2 header layout:
//   [0..7]   magic "KGCLF\x02\x00\x00"
//   [8..11]  version = 2 (uint32 LE)
//   [12..15] num_cat (uint32 LE)
//   [16..19] dim (uint32 LE)
//   [20..23] hidden_dim (uint32 LE)
//   [24..27] name_len (uint32 LE)
//   [28..]   model_name bytes
//   then:    W1[num_cat*dim*hidden], b1[num_cat*hidden],
//            W2[num_cat*hidden], b2[num_cat],
//            W_skip[num_cat*dim],
//            calibration_type (1 byte, 0=raw sigmoid)
struct V2BlobSpec {
    uint32_t num_cat = 8;
    uint32_t dim = 3;
    uint32_t hidden_dim = 2;
    std::string model_name = "test-model";
    std::vector<float> w1;     // [num_cat * dim * hidden_dim]
    std::vector<float> b1;     // [num_cat * hidden_dim]
    std::vector<float> w2;     // [num_cat * hidden_dim]
    std::vector<float> b2;     // [num_cat]
    std::vector<float> w_skip; // [num_cat * dim]
};

std::vector<uint8_t> build_v2_blob(const V2BlobSpec& s) {
    std::vector<uint8_t> blob;

    // Magic: "KGCLF\x02\x00\x00"
    const char magic[] = {'K', 'G', 'C', 'L', 'F', '\x02', '\x00', '\x00'};
    blob.insert(blob.end(), magic, magic + 8);

    auto push_u32 = [&](uint32_t v) {
        blob.push_back(static_cast<uint8_t>(v & 0xff));
        blob.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
        blob.push_back(static_cast<uint8_t>((v >> 16) & 0xff));
        blob.push_back(static_cast<uint8_t>((v >> 24) & 0xff));
    };
    auto push_floats = [&](const std::vector<float>& v) {
        const auto* bytes = reinterpret_cast<const uint8_t*>(v.data());
        blob.insert(blob.end(), bytes, bytes + v.size() * sizeof(float));
    };

    push_u32(2); // format version
    push_u32(s.num_cat);
    push_u32(s.dim);
    push_u32(s.hidden_dim);
    push_u32(static_cast<uint32_t>(s.model_name.size()));
    blob.insert(blob.end(), s.model_name.begin(), s.model_name.end());

    push_floats(s.w1);
    push_floats(s.b1);
    push_floats(s.w2);
    push_floats(s.b2);
    push_floats(s.w_skip);

    // calibration_type = 0 (no Platt scaling)
    blob.push_back(0x00);

    return blob;
}

// Build a v2 blob where all weights are zero. The MLP produces
// sigmoid(0) = 0.5 for every axis, which is below the default
// floor (0.85), so heat_delta = 0. Useful as a "clean" baseline.
std::vector<uint8_t> zero_v2_blob(uint32_t dim = 3, uint32_t hidden = 2, const std::string& model = "test-model") {
    V2BlobSpec s;
    s.num_cat = 8;
    s.dim = dim;
    s.hidden_dim = hidden;
    s.model_name = model;
    s.w1.assign(s.num_cat * s.dim * s.hidden_dim, 0.0f);
    s.b1.assign(s.num_cat * s.hidden_dim, 0.0f);
    s.w2.assign(s.num_cat * s.hidden_dim, 0.0f);
    s.b2.assign(s.num_cat, 0.0f);
    s.w_skip.assign(s.num_cat * s.dim, 0.0f);
    return build_v2_blob(s);
}

// Build a v2 blob with a strong positive weight on a single axis
// via the skip connection. This is the simplest way to make the
// classifier produce a high score on one axis: set W_skip[axis][0]
// to a large positive value, set everything else to zero.
// For an embedding of [1, 0, 0], the logit on that axis becomes
// W_skip[axis][0] * 1.0 = skip_weight, so prob = sigmoid(skip_weight).
//
// With skip_weight = 5.0 and embedding [1,0,0]:
//   logit = 5.0, sigmoid(5.0) ~ 0.9933
// All other axes get logit = 0, sigmoid(0) = 0.5.
std::vector<uint8_t> single_axis_v2_blob(int axis_index, float skip_weight, uint32_t dim = 3, uint32_t hidden = 2,
                                         const std::string& model = "test-model") {
    V2BlobSpec s;
    s.num_cat = 8;
    s.dim = dim;
    s.hidden_dim = hidden;
    s.model_name = model;
    s.w1.assign(s.num_cat * s.dim * s.hidden_dim, 0.0f);
    s.b1.assign(s.num_cat * s.hidden_dim, 0.0f);
    s.w2.assign(s.num_cat * s.hidden_dim, 0.0f);
    s.b2.assign(s.num_cat, 0.0f);
    s.w_skip.assign(s.num_cat * s.dim, 0.0f);
    // W_skip is [num_cat * dim], row-major. Row `axis_index`, col 0.
    s.w_skip[axis_index * dim + 0] = skip_weight;
    return build_v2_blob(s);
}

// Helper to build a config for testing heat_delta_from behavior
// through the check() API. Uses the slur layer as the score source
// since it has floor=0 and produces exact, controllable scores.
ContentModerationConfig slur_pipeline_config() {
    ContentModerationConfig cfg;
    cfg.enabled = true;
    cfg.check_ic = true;
    cfg.check_ooc = true;
    cfg.slurs.enabled = true;
    cfg.slurs.match_score = 1.0;
    // Tight thresholds for test observability.
    cfg.heat.censor_threshold = 0.5;
    cfg.heat.drop_threshold = 2.0;
    cfg.heat.mute_threshold = 5.0;
    cfg.heat.kick_threshold = 10.0;
    cfg.heat.ban_threshold = 15.0;
    cfg.heat.perma_ban_threshold = 25.0;
    cfg.heat.mute_duration_seconds = 60;
    return cfg;
}

} // anonymous namespace

// ===================================================================
// 1. heat_delta_from tests (exercised indirectly via check())
// ===================================================================

// The two-regime formula for heat_delta_from:
//   Above floor: (score - floor) * weight
//   Below floor: score^2 * weight * 0.01
// Tested via the slur axis (floor=0) and the URL blocklist (floor=0)
// which give us exact, controllable scores through check().

TEST_F(ContentModeratorTest, HeatDelta_ScoreZeroContributesNothing) {
    // A clean message with no triggered axes should have zero heat delta.
    cm_.configure(slur_pipeline_config());
    cm_.set_slur_wordlist({"badword"});

    auto v = cm_.check("ipid1", "ooc", "hello friend");
    EXPECT_DOUBLE_EQ(v.heat_delta, 0.0);
    EXPECT_EQ(v.action, ModerationAction::NONE);
}

TEST_F(ContentModeratorTest, HeatDelta_RuleBasedAxisFloorZeroPassesFullScore) {
    // Slur axis has floor=0, so any match passes the full score through.
    // contrib = (score - 0) * weight = score * weight.
    // With match_score=1.0 and weight_slurs=6.0: delta = 1.0 * 6.0 = 6.0.
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 6.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    auto v = cm_.check("ipid1", "ooc", "you badword");
    EXPECT_NEAR(v.heat_delta, 6.0, 1e-9);
    EXPECT_GT(v.scores.slurs, 0.0);
}

TEST_F(ContentModeratorTest, HeatDelta_LinkRiskFullScoreTimesWeight) {
    // URL blocklist also has floor=0 and produces blocked_score=1.0.
    // With weight_link_risk=5.0: delta = 1.0 * 5.0 = 5.0.
    auto cfg = slur_pipeline_config();
    cfg.urls.enabled = true;
    cfg.urls.blocklist = {"evil.com"};
    cfg.urls.blocked_score = 1.0;
    cfg.heat.weight_link_risk = 5.0;
    cm_.configure(cfg);

    auto v = cm_.check("ipid1", "ooc", "visit evil.com now");
    EXPECT_NEAR(v.scores.link_risk, 1.0, 1e-9);
    EXPECT_NEAR(v.heat_delta, 5.0, 1e-9);
}

TEST_F(ContentModeratorTest, HeatDelta_MultipleAxesContributeSimultaneously) {
    // Both slur AND url fire: delta = slur_score*weight_slurs + url_score*weight_link_risk.
    auto cfg = slur_pipeline_config();
    cfg.urls.enabled = true;
    cfg.urls.blocklist = {"evil.com"};
    cfg.urls.blocked_score = 1.0;
    cfg.heat.weight_slurs = 6.0;
    cfg.heat.weight_link_risk = 5.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    auto v = cm_.check("ipid1", "ooc", "badword visit evil.com");
    double expected = 1.0 * 6.0 + 1.0 * 5.0; // slurs + link_risk
    EXPECT_NEAR(v.heat_delta, expected, 1e-9);
}

TEST_F(ContentModeratorTest, HeatDelta_FractionalSlurScore) {
    // Slur layer with match_score=0.5, weight=6.0, floor=0.
    // contrib = 0.5 * 6.0 = 3.0.
    auto cfg = slur_pipeline_config();
    cfg.slurs.match_score = 0.5;
    cfg.heat.weight_slurs = 6.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    auto v = cm_.check("ipid1", "ooc", "you badword");
    EXPECT_NEAR(v.heat_delta, 3.0, 1e-9);
}

TEST_F(ContentModeratorTest, HeatDelta_VisualNoiseFloorZero) {
    // Unicode visual noise has floor=0. A zalgo message with score > 0
    // contributes score * weight_visual_noise.
    auto cfg = slur_pipeline_config();
    cfg.unicode.enabled = true;
    cfg.heat.weight_visual_noise = 2.0;
    cm_.configure(cfg);

    // Build a zalgo string: many combining marks.
    std::string zalgo = "h";
    for (int i = 0; i < 30; ++i)
        zalgo += "\xCC\x80"; // combining grave accent
    zalgo += "i";

    auto v = cm_.check("ipid1", "ooc", zalgo);
    EXPECT_GT(v.scores.visual_noise, 0.0);
    // Heat delta = visual_noise_score * 2.0 (since floor=0 for visual_noise).
    EXPECT_NEAR(v.heat_delta, v.scores.visual_noise * 2.0, 1e-9);
}

// ===================================================================
// 2. Full pipeline integration with mock backend
// ===================================================================

// The local classifier is loaded from embedded assets in configure().
// In tests, embedded assets are empty, so we cannot wire the MLP
// through the ContentModerator public API. Instead, we test the
// full pipeline using rule-based layers (slurs, URLs, unicode) which
// are fully controllable without embedded assets.

TEST_F(ContentModeratorTest, FullPipeline_CleanMessageProducesNone) {
    auto cfg = slur_pipeline_config();
    cfg.unicode.enabled = true;
    cfg.urls.enabled = true;
    cfg.urls.blocklist = {"evil.com"};
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    auto v = cm_.check("ipid1", "ooc", "hello friend how are you");
    EXPECT_EQ(v.action, ModerationAction::NONE);
    EXPECT_DOUBLE_EQ(v.heat_delta, 0.0);
    EXPECT_DOUBLE_EQ(v.scores.slurs, 0.0);
    EXPECT_DOUBLE_EQ(v.scores.link_risk, 0.0);
}

TEST_F(ContentModeratorTest, FullPipeline_SlurTriggersActionAndHeat) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 6.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    auto v = cm_.check("ipid1", "ooc", "you badword");
    // delta = 1.0 * 6.0 = 6.0, which exceeds mute_threshold (5.0).
    EXPECT_GE(static_cast<int>(v.action), static_cast<int>(ModerationAction::MUTE));
    EXPECT_NEAR(v.heat_delta, 6.0, 1e-9);
    EXPECT_GT(v.heat_after, 0.0);
    EXPECT_GT(v.scores.slurs, 0.0);
}

TEST_F(ContentModeratorTest, FullPipeline_CheckProducesCorrectScoresAndReason) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 1.0; // low weight so we only get CENSOR
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    auto v = cm_.check("ipid1", "ooc", "you badword");
    EXPECT_GT(v.scores.slurs, 0.0);
    EXPECT_FALSE(v.triggered_axes.empty());
    // The triggered_axes should contain a slur-related entry.
    bool found_slur = false;
    for (const auto& axis : v.triggered_axes) {
        if (axis.find("slur") != std::string::npos) {
            found_slur = true;
            break;
        }
    }
    EXPECT_TRUE(found_slur);
}

TEST_F(ContentModeratorTest, FullPipeline_NoheatSuppressesEnforcement) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 6.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    cm_.set_noheat("ipid1", true);
    EXPECT_TRUE(cm_.is_noheat("ipid1"));

    auto v = cm_.check("ipid1", "ooc", "you badword");
    // Scores are still computed.
    EXPECT_GT(v.scores.slurs, 0.0);
    EXPECT_GT(v.heat_delta, 0.0);
    // But action is forced to NONE because noheat suppresses enforcement.
    EXPECT_EQ(v.action, ModerationAction::NONE);
    // Heat should NOT have increased.
    EXPECT_DOUBLE_EQ(cm_.current_heat("ipid1"), 0.0);
}

TEST_F(ContentModeratorTest, FullPipeline_NoheatToggle) {
    cm_.configure(slur_pipeline_config());

    EXPECT_FALSE(cm_.is_noheat("ipid1"));
    cm_.set_noheat("ipid1", true);
    EXPECT_TRUE(cm_.is_noheat("ipid1"));
    cm_.set_noheat("ipid1", false);
    EXPECT_FALSE(cm_.is_noheat("ipid1"));
}

// ===================================================================
// 3. Heat ladder escalation
// ===================================================================

TEST_F(ContentModeratorTest, HeatLadder_ProgressionNoneToCensorToDropToMute) {
    // Use URL blocklist with a small weight so we can control escalation.
    // Each blocked URL adds weight_link_risk * 1.0 heat.
    auto cfg = slur_pipeline_config();
    cfg.urls.enabled = true;
    cfg.urls.blocklist = {"bad.com"};
    cfg.urls.blocked_score = 1.0;
    cfg.heat.weight_link_risk = 1.0;
    cfg.heat.censor_threshold = 0.5;
    cfg.heat.drop_threshold = 2.5;
    cfg.heat.mute_threshold = 4.5;
    cfg.heat.kick_threshold = 10.0;
    cfg.heat.ban_threshold = 15.0;
    cfg.heat.perma_ban_threshold = 25.0;
    // Very long half-life so heat doesn't decay during the test.
    cfg.heat.decay_half_life_seconds = 100000.0;
    cm_.configure(cfg);

    // Each message adds ~1.0 heat. Thresholds are spaced to allow
    // clear step transitions without timing sensitivity.

    // Message 1: heat ~1.0, above censor (0.5).
    auto v1 = cm_.check("ipid1", "ooc", "visit bad.com");
    EXPECT_NEAR(v1.heat_delta, 1.0, 1e-9);
    EXPECT_EQ(v1.action, ModerationAction::CENSOR);

    // Message 2: heat ~2.0, still CENSOR (below drop 2.5).
    auto v2 = cm_.check("ipid1", "ooc", "visit bad.com again");
    EXPECT_EQ(v2.action, ModerationAction::CENSOR);

    // Message 3: heat ~3.0, above drop (2.5).
    auto v3 = cm_.check("ipid1", "ooc", "visit bad.com please");
    EXPECT_GE(static_cast<int>(v3.action), static_cast<int>(ModerationAction::DROP));

    // Messages 4-5: heat climbs to ~4.0 and ~5.0, above mute (4.5).
    cm_.check("ipid1", "ooc", "visit bad.com more");
    auto v5 = cm_.check("ipid1", "ooc", "visit bad.com even more");
    EXPECT_GE(static_cast<int>(v5.action), static_cast<int>(ModerationAction::MUTE));
}

TEST_F(ContentModeratorTest, HeatLadder_DifferentAxesEscalateDifferently) {
    // Slurs have higher weight than URLs, so they escalate faster.
    auto cfg = slur_pipeline_config();
    cfg.urls.enabled = true;
    cfg.urls.blocklist = {"bad.com"};
    cfg.urls.blocked_score = 1.0;
    cfg.heat.weight_slurs = 6.0;     // high weight
    cfg.heat.weight_link_risk = 0.3; // low weight
    cfg.heat.censor_threshold = 0.5;
    cfg.heat.mute_threshold = 5.0;
    cfg.heat.decay_half_life_seconds = 100000.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    // A single slur hit jumps straight to MUTE (6.0 > 5.0).
    auto v_slur = cm_.check("ipid_slur", "ooc", "you badword");
    EXPECT_GE(static_cast<int>(v_slur.action), static_cast<int>(ModerationAction::MUTE));

    // URLs at 0.3 per hit: first message heat = 0.3 < censor_threshold 0.5.
    // The low weight means URL infractions take many messages to escalate,
    // while a single slur is immediately catastrophic.
    auto v_url1 = cm_.check("ipid_url", "ooc", "visit bad.com");
    EXPECT_LT(static_cast<int>(v_url1.action), static_cast<int>(v_slur.action));
}

TEST_F(ContentModeratorTest, HeatLadder_HeatMonotonicallyIncreases) {
    auto cfg = slur_pipeline_config();
    cfg.urls.enabled = true;
    cfg.urls.blocklist = {"bad.com"};
    cfg.urls.blocked_score = 1.0;
    cfg.heat.weight_link_risk = 1.0;
    cfg.heat.decay_half_life_seconds = 100000.0;
    cm_.configure(cfg);

    double prev_heat = 0.0;
    for (int i = 0; i < 10; ++i) {
        auto v = cm_.check("ipid1", "ooc", "visit bad.com");
        EXPECT_GT(v.heat_after, prev_heat) << "Heat should increase on message " << i;
        prev_heat = v.heat_after;
    }
}

// ===================================================================
// 4. Mute lifecycle
// ===================================================================

TEST_F(ContentModeratorTest, MuteLifecycle_InitiallyNotMuted) {
    cm_.configure(slur_pipeline_config());
    EXPECT_FALSE(cm_.is_muted("ipid1"));
    EXPECT_FALSE(cm_.get_mute_info("ipid1").has_value());
}

TEST_F(ContentModeratorTest, MuteLifecycle_HighHeatCausesMute) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 6.0;
    cfg.heat.mute_threshold = 5.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    EXPECT_FALSE(cm_.is_muted("ipid1"));
    auto v = cm_.check("ipid1", "ooc", "you badword");
    // heat_delta = 6.0 > mute_threshold = 5.0
    EXPECT_GE(static_cast<int>(v.action), static_cast<int>(ModerationAction::MUTE));
    EXPECT_TRUE(cm_.is_muted("ipid1"));
}

TEST_F(ContentModeratorTest, MuteLifecycle_LiftMuteClearsAndReturnsBool) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 6.0;
    cfg.heat.mute_threshold = 5.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    cm_.check("ipid1", "ooc", "you badword");
    ASSERT_TRUE(cm_.is_muted("ipid1"));

    EXPECT_TRUE(cm_.lift_mute("ipid1"));
    EXPECT_FALSE(cm_.is_muted("ipid1"));

    // Lifting a non-muted IPID returns false.
    EXPECT_FALSE(cm_.lift_mute("ipid1"));
}

TEST_F(ContentModeratorTest, MuteLifecycle_MuteDoesNotAffectOtherIpids) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 6.0;
    cfg.heat.mute_threshold = 5.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    cm_.check("ipid1", "ooc", "you badword");
    EXPECT_TRUE(cm_.is_muted("ipid1"));
    EXPECT_FALSE(cm_.is_muted("ipid2"));
    EXPECT_FALSE(cm_.is_muted("ipid3"));
}

TEST_F(ContentModeratorTest, MuteLifecycle_GetMuteInfoReturnsMetadata) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 6.0;
    cfg.heat.mute_threshold = 5.0;
    cfg.heat.mute_duration_seconds = 300;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    cm_.check("ipid1", "ooc", "you badword");
    auto info = cm_.get_mute_info("ipid1");
    ASSERT_TRUE(info.has_value());
    EXPECT_GT(info->expires_at, 0);
    EXPECT_GT(info->seconds_remaining, 0);
    EXPECT_FALSE(info->reason.empty());
}

TEST_F(ContentModeratorTest, MuteLifecycle_ResetStateClearsMuteAndHeat) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 6.0;
    cfg.heat.mute_threshold = 5.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    cm_.check("ipid1", "ooc", "you badword");
    ASSERT_TRUE(cm_.is_muted("ipid1"));
    EXPECT_GT(cm_.current_heat("ipid1"), 0.0);

    EXPECT_TRUE(cm_.reset_state("ipid1"));
    EXPECT_FALSE(cm_.is_muted("ipid1"));
    EXPECT_DOUBLE_EQ(cm_.current_heat("ipid1"), 0.0);

    // Second reset returns false (nothing to clear).
    EXPECT_FALSE(cm_.reset_state("ipid1"));
}

// ===================================================================
// 5. Edge cases
// ===================================================================

TEST_F(ContentModeratorTest, EdgeCase_EmptyMessageNoCrash) {
    cm_.configure(slur_pipeline_config());
    cm_.set_slur_wordlist({"badword"});

    auto v = cm_.check("ipid1", "ooc", "");
    EXPECT_EQ(v.action, ModerationAction::NONE);
    EXPECT_DOUBLE_EQ(v.heat_delta, 0.0);
}

TEST_F(ContentModeratorTest, EdgeCase_DisabledConfigReturnsNone) {
    ContentModerationConfig cfg;
    cfg.enabled = false;
    cm_.configure(cfg);

    auto v = cm_.check("ipid1", "ooc", "badword evil.com");
    EXPECT_EQ(v.action, ModerationAction::NONE);
    EXPECT_DOUBLE_EQ(v.heat_delta, 0.0);
    EXPECT_DOUBLE_EQ(v.heat_after, 0.0);
}

TEST_F(ContentModeratorTest, EdgeCase_CheckIcFalseReturnsNoneForIC) {
    auto cfg = slur_pipeline_config();
    cfg.check_ic = false;
    cfg.heat.weight_slurs = 6.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    // IC messages should be skipped.
    auto v_ic = cm_.check("ipid1", "ic", "you badword");
    EXPECT_EQ(v_ic.action, ModerationAction::NONE);
    EXPECT_DOUBLE_EQ(v_ic.heat_delta, 0.0);

    // OOC messages should still be checked.
    auto v_ooc = cm_.check("ipid1", "ooc", "you badword");
    EXPECT_GT(v_ooc.heat_delta, 0.0);
    EXPECT_NE(v_ooc.action, ModerationAction::NONE);
}

TEST_F(ContentModeratorTest, EdgeCase_CheckOocFalseReturnsNoneForOOC) {
    auto cfg = slur_pipeline_config();
    cfg.check_ooc = false;
    cfg.heat.weight_slurs = 6.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    // OOC messages should be skipped.
    auto v_ooc = cm_.check("ipid1", "ooc", "you badword");
    EXPECT_EQ(v_ooc.action, ModerationAction::NONE);
    EXPECT_DOUBLE_EQ(v_ooc.heat_delta, 0.0);

    // IC messages should still be checked.
    auto v_ic = cm_.check("ipid1", "ic", "you badword");
    EXPECT_GT(v_ic.heat_delta, 0.0);
    EXPECT_NE(v_ic.action, ModerationAction::NONE);
}

TEST_F(ContentModeratorTest, EdgeCase_UnconfiguredModeratorReturnsNone) {
    // check() without any prior configure() should not crash.
    auto v = cm_.check("ipid1", "ooc", "hello world");
    EXPECT_EQ(v.action, ModerationAction::NONE);
}

TEST_F(ContentModeratorTest, EdgeCase_VeryLongMessageNoCrash) {
    cm_.configure(slur_pipeline_config());
    cm_.set_slur_wordlist({"badword"});

    // Very long clean message. The main assertion is no crash / no
    // out-of-bounds on the various layer scans.
    std::string msg(100000, 'a');
    auto v = cm_.check("ipid1", "ooc", msg);
    // No slur present in a string of 'a' characters; just verify
    // it completes without crashing and returns a valid verdict.
    EXPECT_EQ(v.action, ModerationAction::NONE);
}

TEST_F(ContentModeratorTest, EdgeCase_WhitespaceOnlyMessage) {
    cm_.configure(slur_pipeline_config());
    cm_.set_slur_wordlist({"badword"});

    auto v = cm_.check("ipid1", "ooc", "     \t\n  ");
    EXPECT_EQ(v.action, ModerationAction::NONE);
    EXPECT_DOUBLE_EQ(v.heat_delta, 0.0);
}

TEST_F(ContentModeratorTest, EdgeCase_LiftMuteOnNeverMutedReturnsFalse) {
    cm_.configure(slur_pipeline_config());
    EXPECT_FALSE(cm_.lift_mute("never_muted_ipid"));
}

TEST_F(ContentModeratorTest, EdgeCase_ResetStateOnCleanIpidReturnsFalse) {
    cm_.configure(slur_pipeline_config());
    EXPECT_FALSE(cm_.reset_state("clean_ipid"));
}

// ===================================================================
// 6. Noheat detailed behavior
// ===================================================================

TEST_F(ContentModeratorTest, Noheat_ScoresComputedButHeatUnchanged) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 6.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    cm_.set_noheat("ipid1", true);

    // Send multiple bad messages -- heat should stay at zero.
    for (int i = 0; i < 5; ++i) {
        auto v = cm_.check("ipid1", "ooc", "you badword");
        EXPECT_GT(v.scores.slurs, 0.0);
        EXPECT_GT(v.heat_delta, 0.0);
        EXPECT_EQ(v.action, ModerationAction::NONE);
    }
    EXPECT_DOUBLE_EQ(cm_.current_heat("ipid1"), 0.0);
    EXPECT_FALSE(cm_.is_muted("ipid1"));
}

TEST_F(ContentModeratorTest, Noheat_DisablingRestoresEnforcement) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 6.0;
    cfg.heat.mute_threshold = 5.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    cm_.set_noheat("ipid1", true);
    cm_.check("ipid1", "ooc", "you badword");
    EXPECT_FALSE(cm_.is_muted("ipid1"));

    // Turn noheat off -- next offense should enforce.
    cm_.set_noheat("ipid1", false);
    auto v = cm_.check("ipid1", "ooc", "you badword");
    EXPECT_GE(static_cast<int>(v.action), static_cast<int>(ModerationAction::MUTE));
    EXPECT_TRUE(cm_.is_muted("ipid1"));
}

// ===================================================================
// 7. Audit log integration
// ===================================================================

TEST_F(ContentModeratorTest, AuditLog_CapturesAllFields) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 1.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    ModerationAuditLog audit;
    std::vector<moderation::ModerationEvent> events;
    audit.add_sink("capture", [&events](const moderation::ModerationEvent& ev) { events.push_back(ev); });
    cm_.set_audit_log(&audit);

    cm_.check("ipid1", "ooc", "you badword");
    ASSERT_FALSE(events.empty());

    const auto& ev = events[0];
    EXPECT_EQ(ev.ipid, "ipid1");
    EXPECT_EQ(ev.channel, "ooc");
    EXPECT_GT(ev.scores.slurs, 0.0);
    EXPECT_GT(ev.heat_after, 0.0);
    EXPECT_NE(ev.action, ModerationAction::NONE);
    EXPECT_GT(ev.timestamp_ms, 0);
}

TEST_F(ContentModeratorTest, AuditLog_CleanMessageProducesNoEvent) {
    cm_.configure(slur_pipeline_config());
    cm_.set_slur_wordlist({"badword"});

    ModerationAuditLog audit;
    std::vector<moderation::ModerationEvent> events;
    audit.add_sink("capture", [&events](const moderation::ModerationEvent& ev) { events.push_back(ev); });
    cm_.set_audit_log(&audit);

    cm_.check("ipid1", "ooc", "hello friend");
    // Clean messages produce NONE, which does NOT go through the audit
    // path (heat_delta == 0 short-circuits before audit recording).
    EXPECT_TRUE(events.empty());
}

// ===================================================================
// 8. Action callback integration
// ===================================================================

TEST_F(ContentModeratorTest, ActionCallback_FiredOnNonTrivialVerdict) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 1.0; // enough for CENSOR but not MUTE
    cfg.heat.censor_threshold = 0.5;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    std::string cb_ipid;
    ModerationAction cb_action = ModerationAction::NONE;
    std::string cb_reason;
    cm_.set_action_callback([&](const std::string& ipid, ModerationAction action, const std::string& reason) {
        cb_ipid = ipid;
        cb_action = action;
        cb_reason = reason;
    });

    cm_.check("ipid1", "ooc", "you badword");
    EXPECT_EQ(cb_ipid, "ipid1");
    EXPECT_NE(cb_action, ModerationAction::NONE);
    EXPECT_NE(cb_action, ModerationAction::LOG);
}

TEST_F(ContentModeratorTest, ActionCallback_NotFiredOnCleanMessage) {
    cm_.configure(slur_pipeline_config());

    bool callback_fired = false;
    cm_.set_action_callback([&](const std::string&, ModerationAction, const std::string&) { callback_fired = true; });

    cm_.check("ipid1", "ooc", "hello friend");
    EXPECT_FALSE(callback_fired);
}

// ===================================================================
// 9. Reconfiguration at runtime
// ===================================================================

TEST_F(ContentModeratorTest, Reconfigure_PreservesHeatAcrossReconfigure) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 2.0;
    cfg.heat.decay_half_life_seconds = 100000.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    // Accrue some heat.
    cm_.check("ipid1", "ooc", "you badword");
    double heat_before = cm_.current_heat("ipid1");
    EXPECT_GT(heat_before, 0.0);

    // Reconfigure with different thresholds -- heat should survive.
    cfg.heat.censor_threshold = 100.0; // raise threshold
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    double heat_after = cm_.current_heat("ipid1");
    // Heat should be approximately the same (minor decay is acceptable).
    EXPECT_NEAR(heat_after, heat_before, 0.1);
}

TEST_F(ContentModeratorTest, Reconfigure_PreservesMuteAcrossReconfigure) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 6.0;
    cfg.heat.mute_threshold = 5.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    cm_.check("ipid1", "ooc", "you badword");
    ASSERT_TRUE(cm_.is_muted("ipid1"));

    // Reconfigure -- mute table is rebuilt from DB (which we don't have
    // in this test), so mutes will be cleared. This is expected behavior.
    cm_.configure(cfg);
    // Without a DB, active_mutes_ is cleared in configure().
    EXPECT_FALSE(cm_.is_muted("ipid1"));
}

// ===================================================================
// 10. Sweep / housekeeping
// ===================================================================

TEST_F(ContentModeratorTest, Sweep_DoesNotCrashOnEmptyState) {
    cm_.configure(slur_pipeline_config());
    cm_.sweep(); // no state, no crash
}

TEST_F(ContentModeratorTest, Sweep_DoesNotCrashWithActiveState) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 1.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    cm_.check("ipid1", "ooc", "you badword");
    cm_.sweep(); // should not crash or corrupt state
    // Heat should still be readable.
    EXPECT_GE(cm_.current_heat("ipid1"), 0.0);
}

// ===================================================================
// 11. HeatStats / metrics
// ===================================================================

TEST_F(ContentModeratorTest, HeatStats_TrackedIpidsCountsCorrectly) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 1.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    auto stats0 = cm_.compute_heat_stats();
    EXPECT_EQ(stats0.tracked_ipids, 0u);

    cm_.check("ipid1", "ooc", "you badword");
    cm_.check("ipid2", "ooc", "you badword");

    auto stats1 = cm_.compute_heat_stats();
    EXPECT_GE(stats1.tracked_ipids, 2u);
}

TEST_F(ContentModeratorTest, HeatStats_MutedIpidsCountsCorrectly) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 6.0;
    cfg.heat.mute_threshold = 5.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    auto stats0 = cm_.compute_heat_stats();
    EXPECT_EQ(stats0.muted_ipids, 0u);

    cm_.check("ipid1", "ooc", "you badword");
    cm_.check("ipid2", "ooc", "you badword");

    auto stats1 = cm_.compute_heat_stats();
    EXPECT_EQ(stats1.muted_ipids, 2u);
}

// ===================================================================
// 12. Slur exceptions integration
// ===================================================================

TEST_F(ContentModeratorTest, SlurExceptions_SuppressFalsePositives) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 6.0;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});
    cm_.set_slur_exceptions({"badword"}); // exception cancels the match

    auto v = cm_.check("ipid1", "ooc", "you badword");
    EXPECT_DOUBLE_EQ(v.scores.slurs, 0.0);
    EXPECT_EQ(v.action, ModerationAction::NONE);
}

// ===================================================================
// 13. Mixed channel behavior
// ===================================================================

TEST_F(ContentModeratorTest, MixedChannels_HeatSharedAcrossChannels) {
    auto cfg = slur_pipeline_config();
    cfg.heat.weight_slurs = 2.0;
    cfg.heat.decay_half_life_seconds = 100000.0;
    cfg.check_ic = true;
    cfg.check_ooc = true;
    cm_.configure(cfg);
    cm_.set_slur_wordlist({"badword"});

    cm_.check("ipid1", "ooc", "you badword");
    double heat_after_ooc = cm_.current_heat("ipid1");

    cm_.check("ipid1", "ic", "you badword");
    double heat_after_ic = cm_.current_heat("ipid1");

    // Heat from IC should stack on top of OOC heat.
    EXPECT_GT(heat_after_ic, heat_after_ooc);
}
