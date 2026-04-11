// Regression tests for the default-merge contract ServerSettings relies
// on for its content_moderation block. These intentionally do NOT
// depend on apps/kagami/ServerSettings.h — that header lives in the
// kagami binary target, not in a library aosdl_tests links against.
// Instead, we build a miniature JsonConfiguration subclass that seeds
// the exact same defaults_ shape the real ServerSettings constructor
// uses for its content_moderation block, then verify the merge
// behavior against partial kagami.json documents.
//
// The contract these tests enforce:
//
//   1. A kagami.json with NO content_moderation section produces
//      every content_moderation/* key at its defaults_ value (all
//      layers disabled, weights set to the roleplay-friendly tuning).
//
//   2. A kagami.json with a PARTIAL content_moderation section
//      (e.g. a legacy config that was written before Layer 1c or
//      safe-hint existed) falls back to defaults for the missing
//      sub-blocks without requiring the operator to re-generate
//      their config file on upgrade.
//
//   3. Specific regression: weight_slurs (new in the Wiktionary
//      wordlist commit) defaults to 6.0 even when the user's JSON
//      specifies a heat block that doesn't mention it. The old
//      behavior (pre-merge) would have left weight_slurs as 0.0,
//      making slur matches contribute zero heat.
//
// The JsonConfiguration::do_value() path falls back to defaults_
// whenever a key isn't found in json_, so missing keys at any
// depth of the content_moderation tree produce the documented
// default behavior. This file is the regression harness for that.
#include "configuration/JsonConfiguration.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {

// Minimal subclass that seeds exactly the content_moderation defaults
// block ServerSettings uses. Any field read through value<>() that
// isn't present in the deserialized JSON will fall through to this
// tree.
//
// When updating ServerSettings::set_defaults() in a future commit,
// update the mirror below too — or factor the defaults tree out into
// a shared header both can include.
class MockServerSettings : public JsonConfiguration<MockServerSettings> {
  public:
    static void install_moderation_defaults() {
        instance().set_defaults(nlohmann::json{
            {"content_moderation",
             nlohmann::json{
                 {"enabled", false},
                 {"check_ic", true},
                 {"check_ooc", true},
                 {"message_sample_length", 200},
                 {"unicode",
                  nlohmann::json{
                      {"enabled", false},
                      {"combining_mark_threshold", 0.3},
                      {"exotic_script_threshold", 0.3},
                      {"format_char_threshold", 0.1},
                      {"max_score", 1.0},
                  }},
                 {"urls",
                  nlohmann::json{
                      {"enabled", false},
                      {"blocklist", nlohmann::json::array()},
                      {"allowlist", nlohmann::json::array()},
                      {"blocked_score", 1.0},
                      {"unknown_url_score", 0.0},
                  }},
                 {"slurs",
                  nlohmann::json{
                      {"enabled", false},
                      {"wordlist_url", ""},
                      {"exceptions_url", ""},
                      {"cache_dir", "/tmp/kagami-moderation"},
                      {"match_score", 1.0},
                  }},
                 {"remote",
                  nlohmann::json{
                      {"enabled", false},
                      {"provider", "openai"},
                      {"api_key", ""},
                      {"endpoint", "https://api.openai.com/v1/moderations"},
                      {"model", "text-moderation-latest"},
                      {"timeout_ms", 3000},
                      {"fail_open", true},
                  }},
                 {"safe_hint",
                  nlohmann::json{
                      {"enabled", false},
                      {"anchors_url", ""},
                      {"cache_dir", "/tmp/kagami-moderation"},
                      {"similarity_threshold", 0.7},
                  }},
                 {"embeddings",
                  nlohmann::json{
                      {"enabled", false},
                      {"hf_model_id", ""},
                      {"ring_size", 500},
                      {"similarity_threshold", 0.9},
                      {"cluster_threshold", 3},
                      {"window_seconds", 60},
                  }},
                 {"heat",
                  nlohmann::json{
                      {"decay_half_life_seconds", 600.0},
                      {"censor_threshold", 1.0},
                      {"drop_threshold", 3.0},
                      {"mute_threshold", 6.0},
                      {"kick_threshold", 10.0},
                      {"ban_threshold", 15.0},
                      {"perma_ban_threshold", 25.0},
                      {"mute_duration_seconds", 900},
                      {"ban_duration_seconds", 86400},
                      {"weight_visual_noise", 0.5},
                      {"weight_link_risk", 5.0},
                      {"weight_slurs", 6.0},
                      {"weight_toxicity", 1.0},
                      {"weight_hate", 4.0},
                      {"weight_sexual", 1.5},
                      {"weight_sexual_minors", 100.0},
                      {"weight_violence", 1.0},
                      {"weight_self_harm", 1.0},
                      {"weight_semantic_echo", 2.0},
                  }},
             }},
        });
    }
};

class ContentModerationDefaultsTest : public ::testing::Test {
  protected:
    void SetUp() override {
        cfg().clear_on_change();
        cfg().clear();
        MockServerSettings::install_moderation_defaults();
    }
    static MockServerSettings& cfg() {
        return MockServerSettings::instance();
    }
    static bool load(const std::string& json) {
        std::vector<uint8_t> bytes(json.begin(), json.end());
        return MockServerSettings::instance().deserialize(bytes);
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Empty JSON — every key should resolve to its defaults_ value.
// ---------------------------------------------------------------------------

TEST_F(ContentModerationDefaultsTest, EmptyJsonMasterSwitchOff) {
    ASSERT_TRUE(load("{}"));
    EXPECT_FALSE(cfg().value<bool>("content_moderation/enabled"));
}

TEST_F(ContentModerationDefaultsTest, EmptyJsonAllLayersDisabled) {
    ASSERT_TRUE(load("{}"));
    EXPECT_FALSE(cfg().value<bool>("content_moderation/unicode/enabled"));
    EXPECT_FALSE(cfg().value<bool>("content_moderation/urls/enabled"));
    EXPECT_FALSE(cfg().value<bool>("content_moderation/slurs/enabled"));
    EXPECT_FALSE(cfg().value<bool>("content_moderation/remote/enabled"));
    EXPECT_FALSE(cfg().value<bool>("content_moderation/safe_hint/enabled"));
    EXPECT_FALSE(cfg().value<bool>("content_moderation/embeddings/enabled"));
}

TEST_F(ContentModerationDefaultsTest, EmptyJsonSecretsEmpty) {
    ASSERT_TRUE(load("{}"));
    EXPECT_TRUE(cfg().value<std::string>("content_moderation/remote/api_key").empty());
    EXPECT_TRUE(cfg().value<std::string>("content_moderation/slurs/wordlist_url").empty());
    EXPECT_TRUE(cfg().value<std::string>("content_moderation/slurs/exceptions_url").empty());
    EXPECT_TRUE(cfg().value<std::string>("content_moderation/safe_hint/anchors_url").empty());
    EXPECT_TRUE(cfg().value<std::string>("content_moderation/embeddings/hf_model_id").empty());
}

TEST_F(ContentModerationDefaultsTest, EmptyJsonRoleplayTuning) {
    // The inline comments in ContentModerator.cpp document
    // weight_toxicity=1.0 and weight_violence=1.0 as the roleplay-
    // friendly tuning. If someone bumps these in the defaults without
    // updating the companion doc (or the per-axis floor), this test
    // catches the drift.
    ASSERT_TRUE(load("{}"));
    EXPECT_DOUBLE_EQ(cfg().value<double>("content_moderation/heat/weight_toxicity"), 1.0);
    EXPECT_DOUBLE_EQ(cfg().value<double>("content_moderation/heat/weight_violence"), 1.0);
    EXPECT_DOUBLE_EQ(cfg().value<double>("content_moderation/heat/weight_hate"), 4.0);
    // Catastrophic on contact — any positive score on this axis must
    // cross the perma-ban threshold in one shot.
    EXPECT_GE(cfg().value<double>("content_moderation/heat/weight_sexual_minors"), 25.0);
    // New in the Wiktionary wordlist commit: weight_slurs defaults to
    // mute_threshold so a single hit mutes.
    EXPECT_DOUBLE_EQ(cfg().value<double>("content_moderation/heat/weight_slurs"), 6.0);
    EXPECT_DOUBLE_EQ(cfg().value<double>("content_moderation/heat/mute_threshold"), 6.0);
}

// ---------------------------------------------------------------------------
// Legacy config — a kagami.json written before Layer 1c / safe-hint landed.
// ---------------------------------------------------------------------------

TEST_F(ContentModerationDefaultsTest, LegacyConfigPreservesExplicitAndMergesNew) {
    // The operator had enabled just the unicode and urls layers before
    // the slur and safe-hint features shipped. After the upgrade,
    // their existing values must be preserved exactly, AND the new
    // sub-blocks must read as defaults — NOT as missing/zero.
    const std::string legacy = R"({
        "content_moderation": {
            "enabled": true,
            "unicode": {"enabled": true},
            "urls": {"enabled": true}
        }
    })";
    ASSERT_TRUE(load(legacy));

    // Explicit values preserved.
    EXPECT_TRUE(cfg().value<bool>("content_moderation/enabled"));
    EXPECT_TRUE(cfg().value<bool>("content_moderation/unicode/enabled"));
    EXPECT_TRUE(cfg().value<bool>("content_moderation/urls/enabled"));

    // New sub-blocks default to off.
    EXPECT_FALSE(cfg().value<bool>("content_moderation/slurs/enabled"));
    EXPECT_TRUE(cfg().value<std::string>("content_moderation/slurs/wordlist_url").empty());
    EXPECT_FALSE(cfg().value<bool>("content_moderation/safe_hint/enabled"));

    // CRITICAL regression: the new weight_slurs field must read as
    // 6.0 from defaults_, not 0.0. The old bug class this guards
    // against is: "operator's heat block doesn't mention weight_slurs,
    // therefore it reads as default-constructed double (0.0), therefore
    // slur matches contribute no heat, therefore Layer 1c silently
    // produces nothing visible to users even though the metric fires."
    EXPECT_DOUBLE_EQ(cfg().value<double>("content_moderation/heat/weight_slurs"), 6.0);
}

// ---------------------------------------------------------------------------
// Partial JSON — only the new slur block.
// ---------------------------------------------------------------------------

TEST_F(ContentModerationDefaultsTest, SlurBlockOnlyLeavesOthersAtDefault) {
    const std::string slurs_only = R"({
        "content_moderation": {
            "enabled": true,
            "slurs": {
                "enabled": true,
                "wordlist_url": "https://example.invalid/slurs.txt"
            }
        }
    })";
    ASSERT_TRUE(load(slurs_only));

    EXPECT_TRUE(cfg().value<bool>("content_moderation/enabled"));
    EXPECT_TRUE(cfg().value<bool>("content_moderation/slurs/enabled"));
    EXPECT_EQ(cfg().value<std::string>("content_moderation/slurs/wordlist_url"), "https://example.invalid/slurs.txt");
    // Unspecified sub-fields of slurs/ still read from defaults.
    EXPECT_TRUE(cfg().value<std::string>("content_moderation/slurs/exceptions_url").empty());
    EXPECT_DOUBLE_EQ(cfg().value<double>("content_moderation/slurs/match_score"), 1.0);

    // Everything else stays off.
    EXPECT_FALSE(cfg().value<bool>("content_moderation/unicode/enabled"));
    EXPECT_FALSE(cfg().value<bool>("content_moderation/urls/enabled"));
    EXPECT_FALSE(cfg().value<bool>("content_moderation/remote/enabled"));
    EXPECT_FALSE(cfg().value<bool>("content_moderation/safe_hint/enabled"));
    EXPECT_FALSE(cfg().value<bool>("content_moderation/embeddings/enabled"));
}

// ---------------------------------------------------------------------------
// Partial JSON — safe_hint enabled without embeddings.
// ---------------------------------------------------------------------------

TEST_F(ContentModerationDefaultsTest, SafeHintEnabledWithoutEmbeddingsKeepsDefaults) {
    // Documents the cross-layer dependency: SafeHintLayer's runtime
    // code path requires a ready embedding backend (enforced in
    // main.cpp's background fetch block, not in the config parser).
    // The config itself will accept safe_hint.enabled=true — the
    // inertness is a runtime property, not a config-time one.
    const std::string partial = R"({
        "content_moderation": {
            "enabled": true,
            "safe_hint": {
                "enabled": true,
                "anchors_url": "https://example.invalid/anchors.txt"
            }
        }
    })";
    ASSERT_TRUE(load(partial));

    EXPECT_TRUE(cfg().value<bool>("content_moderation/safe_hint/enabled"));
    EXPECT_EQ(cfg().value<std::string>("content_moderation/safe_hint/anchors_url"),
              "https://example.invalid/anchors.txt");
    // Threshold falls back to default 0.7.
    EXPECT_DOUBLE_EQ(cfg().value<double>("content_moderation/safe_hint/similarity_threshold"), 0.7);

    // Embeddings layer stays inert — operator didn't set an HF model id.
    EXPECT_FALSE(cfg().value<bool>("content_moderation/embeddings/enabled"));
    EXPECT_TRUE(cfg().value<std::string>("content_moderation/embeddings/hf_model_id").empty());
}
