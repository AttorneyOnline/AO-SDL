#include "moderation/ModerationTrace.h"

#include <gtest/gtest.h>
#include <json.hpp>

#include <string>

namespace {

using moderation::ModerationAction;
using moderation::ModerationTrace;
using moderation::trace_to_json_line;

ModerationTrace make_trace() {
    ModerationTrace t;
    t.timestamp_ms = 1700000000000LL;
    t.ipid = "abc123";
    t.channel = "ic";
    t.message = "hello everyone";
    return t;
}

} // namespace

TEST(ModerationTraceTest, EmptyTraceRoundTrips) {
    auto t = make_trace();
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_EQ(j["ts"].get<int64_t>(), 1700000000000LL);
    EXPECT_EQ(j["ipid"].get<std::string>(), "abc123");
    EXPECT_EQ(j["channel"].get<std::string>(), "ic");
    EXPECT_EQ(j["message"].get<std::string>(), "hello everyone");
    EXPECT_FALSE(j["layers"]["unicode"]["ran"].get<bool>());
    EXPECT_FALSE(j["layers"]["urls"]["ran"].get<bool>());
    EXPECT_FALSE(j["layers"]["slurs"]["ran"].get<bool>());
    EXPECT_FALSE(j["layers"]["local_classifier"]["ran"].get<bool>());
    EXPECT_FALSE(j["layers"]["bad_hint"]["ran"].get<bool>());
    EXPECT_FALSE(j["decision"]["keysmash_suppressed"].get<bool>());
    EXPECT_EQ(j["decision"]["final_action"].get<std::string>(), "NONE");
}

TEST(ModerationTraceTest, PopulatedLayersRoundTrip) {
    auto t = make_trace();
    t.unicode.ran = true;
    t.unicode.ns = 120;
    t.unicode.visual_noise = 0.35;

    t.urls.ran = true;
    t.urls.ns = 80;
    t.urls.link_risk = 0.0;
    t.urls.urls = {"https://example.com", "http://wiki.example.org"};

    t.slurs.ran = true;
    t.slurs.ns = 340;
    t.slurs.match_score = 1.0;
    t.slurs.matches = {"slur_token"};

    t.local_classifier.ran = true;
    t.local_classifier.ns = 210;
    t.local_classifier.max_confidence = 0.87;
    t.local_classifier.max_category_index = 1;
    t.local_classifier.scores.hate = 0.87;
    t.local_classifier.scores.sexual = 0.65;

    t.bad_hint.ran = true;
    t.bad_hint.ns = 410;
    t.bad_hint.max_similarity = 0.82;
    t.bad_hint.best_anchor_index = 7;
    t.bad_hint.is_bad = true;

    t.keysmash_suppressed = false;
    t.triggered_axes = {"sexual_lc", "hate_lc"};
    t.final_scores.hate = 0.87;
    t.final_scores.sexual = 0.65;
    t.heat_before = 0.0;
    t.heat_delta = 6.0;
    t.heat_after = 6.0;
    t.final_action = ModerationAction::MUTE;
    t.reason = "hate=0.87,sexual=0.65";

    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);

    EXPECT_TRUE(j["layers"]["unicode"]["ran"].get<bool>());
    EXPECT_DOUBLE_EQ(j["layers"]["unicode"]["visual_noise"].get<double>(), 0.35);
    EXPECT_EQ(j["layers"]["urls"]["urls"].size(), 2u);
    EXPECT_EQ(j["layers"]["urls"]["urls"][0].get<std::string>(), "https://example.com");
    EXPECT_EQ(j["layers"]["slurs"]["matches"].size(), 1u);
    EXPECT_EQ(j["layers"]["slurs"]["matches"][0].get<std::string>(), "slur_token");
    EXPECT_TRUE(j["layers"]["local_classifier"]["ran"].get<bool>());
    EXPECT_DOUBLE_EQ(j["layers"]["local_classifier"]["max_confidence"].get<double>(), 0.87);
    EXPECT_EQ(j["layers"]["local_classifier"]["max_category_index"].get<int>(), 1);
    EXPECT_DOUBLE_EQ(j["layers"]["local_classifier"]["scores"]["hate"].get<double>(), 0.87);
    EXPECT_TRUE(j["layers"]["bad_hint"]["is_bad"].get<bool>());
    EXPECT_EQ(j["layers"]["bad_hint"]["best_anchor_index"].get<int>(), 7);
    EXPECT_FALSE(j["decision"]["keysmash_suppressed"].get<bool>());
    EXPECT_EQ(j["decision"]["final_action"].get<std::string>(), "MUTE");
    EXPECT_EQ(j["decision"]["triggered_axes"].size(), 2u);
    EXPECT_DOUBLE_EQ(j["decision"]["heat_delta"].get<double>(), 6.0);
    EXPECT_DOUBLE_EQ(j["decision"]["heat_after"].get<double>(), 6.0);
}

TEST(ModerationTraceTest, KeysmashSuppressedSerialization) {
    auto t = make_trace();
    t.keysmash_suppressed = true;
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_TRUE(j["decision"]["keysmash_suppressed"].get<bool>());
}

TEST(ModerationTraceTest, ActionStringifies) {
    const std::pair<ModerationAction, const char*> cases[] = {
        {ModerationAction::NONE, "NONE"},     {ModerationAction::LOG, "LOG"},
        {ModerationAction::CENSOR, "CENSOR"}, {ModerationAction::DROP, "DROP"},
        {ModerationAction::MUTE, "MUTE"},     {ModerationAction::KICK, "KICK"},
        {ModerationAction::BAN, "BAN"},       {ModerationAction::PERMA_BAN, "PERMA_BAN"},
    };
    for (auto [action, expected] : cases) {
        auto t = make_trace();
        t.final_action = action;
        auto line = trace_to_json_line(t);
        auto j = nlohmann::json::parse(line);
        EXPECT_EQ(j["decision"]["final_action"].get<std::string>(), expected)
            << "action " << static_cast<int>(action) << " stringified wrong";
    }
}

TEST(ModerationTraceTest, AllAxesEmittedEvenAtZero) {
    auto t = make_trace();
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    const char* axes[] = {"visual_noise",  "link_risk", "slurs",     "hate",         "sexual",
                          "sexual_minors", "violence",  "self_harm", "semantic_echo"};
    for (const char* axis : axes) {
        EXPECT_TRUE(j["decision"]["final_scores"].contains(axis)) << "final_scores missing axis: " << axis;
        EXPECT_TRUE(j["layers"]["local_classifier"]["scores"].contains(axis))
            << "local_classifier.scores missing axis: " << axis;
    }
}

// ===========================================================================
// Full trace with all fields populated — verify every key appears in JSON.
// ===========================================================================

TEST(ModerationTraceTest, FullTraceAllFieldsPresent) {
    ModerationTrace t;
    t.timestamp_ms = 1700000000123LL;
    t.ipid = "full_test_ipid";
    t.channel = "ooc";
    t.area = "courtroom_1";
    t.message = "test message with all fields";

    t.unicode.ran = true;
    t.unicode.ns = 500;
    t.unicode.visual_noise = 0.42;

    t.urls.ran = true;
    t.urls.ns = 300;
    t.urls.link_risk = 0.75;
    t.urls.urls = {"https://evil.example.com"};

    t.slurs.ran = true;
    t.slurs.ns = 150;
    t.slurs.match_score = 1.0;
    t.slurs.matches = {"token_a", "token_b"};

    t.layer2_embedding.ran = true;
    t.layer2_embedding.ns = 12000;
    t.layer2_embedding.dim = 384;

    t.local_classifier.ran = true;
    t.local_classifier.ns = 800;
    t.local_classifier.max_confidence = 0.95;
    t.local_classifier.max_category_index = 3;
    t.local_classifier.scores.hate = 0.95;
    t.local_classifier.scores.sexual = 0.10;
    t.local_classifier.scores.sexual_minors = 0.01;
    t.local_classifier.scores.violence = 0.30;
    t.local_classifier.scores.self_harm = 0.05;

    t.bad_hint.ran = true;
    t.bad_hint.ns = 600;
    t.bad_hint.max_similarity = 0.91;
    t.bad_hint.best_anchor_index = 12;
    t.bad_hint.is_bad = true;

    t.semantic_cluster.ran = true;
    t.semantic_cluster.ns = 2000;
    t.semantic_cluster.semantic_echo = 0.67;
    t.semantic_cluster.cluster_size = 5;

    t.keysmash_suppressed = false;
    t.noheat_suppressed = true;
    t.skip_reason = "none";
    t.triggered_axes = {"hate_lc", "violence_lc"};
    t.final_scores.hate = 0.95;
    t.final_scores.violence = 0.30;
    t.heat_before = 2.0;
    t.heat_delta = 9.5;
    t.heat_after = 11.5;
    t.final_action = ModerationAction::KICK;
    t.reason = "hate=0.95,violence=0.30";

    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);

    // Top-level envelope
    EXPECT_EQ(j["ts"].get<int64_t>(), 1700000000123LL);
    EXPECT_EQ(j["ipid"].get<std::string>(), "full_test_ipid");
    EXPECT_EQ(j["channel"].get<std::string>(), "ooc");
    EXPECT_EQ(j["area"].get<std::string>(), "courtroom_1");
    EXPECT_EQ(j["message"].get<std::string>(), "test message with all fields");

    // Unicode layer
    EXPECT_TRUE(j["layers"]["unicode"]["ran"].get<bool>());
    EXPECT_EQ(j["layers"]["unicode"]["ns"].get<int64_t>(), 500);
    EXPECT_DOUBLE_EQ(j["layers"]["unicode"]["visual_noise"].get<double>(), 0.42);

    // URLs layer
    EXPECT_TRUE(j["layers"]["urls"]["ran"].get<bool>());
    EXPECT_EQ(j["layers"]["urls"]["ns"].get<int64_t>(), 300);
    EXPECT_DOUBLE_EQ(j["layers"]["urls"]["link_risk"].get<double>(), 0.75);
    EXPECT_EQ(j["layers"]["urls"]["urls"].size(), 1u);
    EXPECT_EQ(j["layers"]["urls"]["urls"][0].get<std::string>(), "https://evil.example.com");

    // Slurs layer
    EXPECT_TRUE(j["layers"]["slurs"]["ran"].get<bool>());
    EXPECT_DOUBLE_EQ(j["layers"]["slurs"]["match_score"].get<double>(), 1.0);
    EXPECT_EQ(j["layers"]["slurs"]["matches"].size(), 2u);
    EXPECT_EQ(j["layers"]["slurs"]["matches"][0].get<std::string>(), "token_a");
    EXPECT_EQ(j["layers"]["slurs"]["matches"][1].get<std::string>(), "token_b");

    // Layer 2 embedding
    EXPECT_TRUE(j["layers"]["layer2_embedding"]["ran"].get<bool>());
    EXPECT_EQ(j["layers"]["layer2_embedding"]["ns"].get<int64_t>(), 12000);
    EXPECT_EQ(j["layers"]["layer2_embedding"]["dim"].get<int>(), 384);

    // Local classifier
    EXPECT_TRUE(j["layers"]["local_classifier"]["ran"].get<bool>());
    EXPECT_DOUBLE_EQ(j["layers"]["local_classifier"]["max_confidence"].get<double>(), 0.95);
    EXPECT_EQ(j["layers"]["local_classifier"]["max_category_index"].get<int>(), 3);
    EXPECT_DOUBLE_EQ(j["layers"]["local_classifier"]["scores"]["hate"].get<double>(), 0.95);
    EXPECT_DOUBLE_EQ(j["layers"]["local_classifier"]["scores"]["violence"].get<double>(), 0.30);

    // Bad hint
    EXPECT_TRUE(j["layers"]["bad_hint"]["ran"].get<bool>());
    EXPECT_DOUBLE_EQ(j["layers"]["bad_hint"]["max_similarity"].get<double>(), 0.91);
    EXPECT_EQ(j["layers"]["bad_hint"]["best_anchor_index"].get<int>(), 12);
    EXPECT_TRUE(j["layers"]["bad_hint"]["is_bad"].get<bool>());

    // Semantic cluster
    EXPECT_TRUE(j["layers"]["semantic_cluster"]["ran"].get<bool>());
    EXPECT_EQ(j["layers"]["semantic_cluster"]["ns"].get<int64_t>(), 2000);
    EXPECT_DOUBLE_EQ(j["layers"]["semantic_cluster"]["semantic_echo"].get<double>(), 0.67);
    EXPECT_EQ(j["layers"]["semantic_cluster"]["cluster_size"].get<int>(), 5);

    // Decision block
    EXPECT_FALSE(j["decision"]["keysmash_suppressed"].get<bool>());
    EXPECT_TRUE(j["decision"]["noheat_suppressed"].get<bool>());
    EXPECT_EQ(j["decision"]["triggered_axes"].size(), 2u);
    EXPECT_EQ(j["decision"]["final_action"].get<std::string>(), "KICK");
    EXPECT_DOUBLE_EQ(j["decision"]["heat_before"].get<double>(), 2.0);
    EXPECT_DOUBLE_EQ(j["decision"]["heat_delta"].get<double>(), 9.5);
    EXPECT_DOUBLE_EQ(j["decision"]["heat_after"].get<double>(), 11.5);
    EXPECT_EQ(j["decision"]["reason"].get<std::string>(), "hate=0.95,violence=0.30");
}

// ===========================================================================
// Noheat suppressed flag
// ===========================================================================

TEST(ModerationTraceTest, NoheatSuppressedFlagSerialization) {
    auto t = make_trace();
    t.noheat_suppressed = true;
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_TRUE(j["decision"]["noheat_suppressed"].get<bool>());
}

TEST(ModerationTraceTest, NoheatDefaultFalse) {
    auto t = make_trace();
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_FALSE(j["decision"]["noheat_suppressed"].get<bool>());
}

// ===========================================================================
// Unicode layer trace
// ===========================================================================

TEST(ModerationTraceTest, UnicodeLayerTraceWhenNotRun) {
    auto t = make_trace();
    t.unicode.ran = false;
    t.unicode.ns = 0;
    t.unicode.visual_noise = 0.0;
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_FALSE(j["layers"]["unicode"]["ran"].get<bool>());
    EXPECT_EQ(j["layers"]["unicode"]["ns"].get<int64_t>(), 0);
    EXPECT_DOUBLE_EQ(j["layers"]["unicode"]["visual_noise"].get<double>(), 0.0);
}

// ===========================================================================
// URLs layer trace with empty URL list
// ===========================================================================

TEST(ModerationTraceTest, UrlsLayerEmptyUrlList) {
    auto t = make_trace();
    t.urls.ran = true;
    t.urls.ns = 50;
    t.urls.link_risk = 0.0;
    // No URLs
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_TRUE(j["layers"]["urls"]["ran"].get<bool>());
    EXPECT_EQ(j["layers"]["urls"]["urls"].size(), 0u);
}

TEST(ModerationTraceTest, UrlsLayerMultipleUrls) {
    auto t = make_trace();
    t.urls.ran = true;
    t.urls.urls = {"https://a.com", "https://b.com", "https://c.com"};
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_EQ(j["layers"]["urls"]["urls"].size(), 3u);
}

// ===========================================================================
// Slurs layer trace
// ===========================================================================

TEST(ModerationTraceTest, SlursLayerNoMatches) {
    auto t = make_trace();
    t.slurs.ran = true;
    t.slurs.ns = 100;
    t.slurs.match_score = 0.0;
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_TRUE(j["layers"]["slurs"]["ran"].get<bool>());
    EXPECT_EQ(j["layers"]["slurs"]["matches"].size(), 0u);
    EXPECT_DOUBLE_EQ(j["layers"]["slurs"]["match_score"].get<double>(), 0.0);
}

TEST(ModerationTraceTest, SlursLayerMultipleMatches) {
    auto t = make_trace();
    t.slurs.ran = true;
    t.slurs.matches = {"word_a", "word_b", "word_c"};
    t.slurs.match_score = 1.0;
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_EQ(j["layers"]["slurs"]["matches"].size(), 3u);
    EXPECT_EQ(j["layers"]["slurs"]["matches"][2].get<std::string>(), "word_c");
}

// ===========================================================================
// Local classifier trace
// ===========================================================================

TEST(ModerationTraceTest, LocalClassifierScoresAndTiming) {
    auto t = make_trace();
    t.local_classifier.ran = true;
    t.local_classifier.ns = 5000;
    t.local_classifier.max_confidence = 0.72;
    t.local_classifier.max_category_index = 4;
    t.local_classifier.scores.sexual = 0.72;
    t.local_classifier.scores.hate = 0.15;

    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_TRUE(j["layers"]["local_classifier"]["ran"].get<bool>());
    EXPECT_EQ(j["layers"]["local_classifier"]["ns"].get<int64_t>(), 5000);
    EXPECT_DOUBLE_EQ(j["layers"]["local_classifier"]["max_confidence"].get<double>(), 0.72);
    EXPECT_EQ(j["layers"]["local_classifier"]["max_category_index"].get<int>(), 4);
    EXPECT_DOUBLE_EQ(j["layers"]["local_classifier"]["scores"]["sexual"].get<double>(), 0.72);
    EXPECT_DOUBLE_EQ(j["layers"]["local_classifier"]["scores"]["hate"].get<double>(), 0.15);
}

TEST(ModerationTraceTest, LocalClassifierNotRunShowsDefaults) {
    auto t = make_trace();
    // local_classifier not set — defaults apply
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_FALSE(j["layers"]["local_classifier"]["ran"].get<bool>());
    EXPECT_EQ(j["layers"]["local_classifier"]["ns"].get<int64_t>(), 0);
    EXPECT_DOUBLE_EQ(j["layers"]["local_classifier"]["max_confidence"].get<double>(), 0.0);
    EXPECT_EQ(j["layers"]["local_classifier"]["max_category_index"].get<int>(), -1);
}

// ===========================================================================
// Bad hint trace
// ===========================================================================

TEST(ModerationTraceTest, BadHintNotBad) {
    auto t = make_trace();
    t.bad_hint.ran = true;
    t.bad_hint.ns = 200;
    t.bad_hint.max_similarity = 0.45;
    t.bad_hint.best_anchor_index = 3;
    t.bad_hint.is_bad = false;

    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_TRUE(j["layers"]["bad_hint"]["ran"].get<bool>());
    EXPECT_FALSE(j["layers"]["bad_hint"]["is_bad"].get<bool>());
    EXPECT_EQ(j["layers"]["bad_hint"]["best_anchor_index"].get<int>(), 3);
    EXPECT_DOUBLE_EQ(j["layers"]["bad_hint"]["max_similarity"].get<double>(), 0.45);
}

TEST(ModerationTraceTest, BadHintDefaults) {
    auto t = make_trace();
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_FALSE(j["layers"]["bad_hint"]["ran"].get<bool>());
    EXPECT_FALSE(j["layers"]["bad_hint"]["is_bad"].get<bool>());
    EXPECT_EQ(j["layers"]["bad_hint"]["best_anchor_index"].get<int>(), -1);
    EXPECT_DOUBLE_EQ(j["layers"]["bad_hint"]["max_similarity"].get<double>(), 0.0);
}

// ===========================================================================
// Semantic cluster trace
// ===========================================================================

TEST(ModerationTraceTest, SemanticClusterSerialization) {
    auto t = make_trace();
    t.semantic_cluster.ran = true;
    t.semantic_cluster.ns = 900;
    t.semantic_cluster.semantic_echo = 0.88;
    t.semantic_cluster.cluster_size = 7;

    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_TRUE(j["layers"]["semantic_cluster"]["ran"].get<bool>());
    EXPECT_DOUBLE_EQ(j["layers"]["semantic_cluster"]["semantic_echo"].get<double>(), 0.88);
    EXPECT_EQ(j["layers"]["semantic_cluster"]["cluster_size"].get<int>(), 7);
}

// ===========================================================================
// All axes emitted even at zero — verify in both final_scores AND
// local_classifier.scores at the same time.
// ===========================================================================

TEST(ModerationTraceTest, AllAxesZeroAreExplicitlyPresent) {
    auto t = make_trace();
    // Everything left at defaults (all zeros)
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);

    const char* axes[] = {"visual_noise",  "link_risk", "slurs",     "hate",         "sexual",
                          "sexual_minors", "violence",  "self_harm", "semantic_echo"};
    for (const char* axis : axes) {
        ASSERT_TRUE(j["decision"]["final_scores"].contains(axis)) << "final_scores missing zero axis: " << axis;
        EXPECT_DOUBLE_EQ(j["decision"]["final_scores"][axis].get<double>(), 0.0)
            << "final_scores[" << axis << "] should be 0.0";

        ASSERT_TRUE(j["layers"]["local_classifier"]["scores"].contains(axis))
            << "local_classifier.scores missing zero axis: " << axis;
        EXPECT_DOUBLE_EQ(j["layers"]["local_classifier"]["scores"][axis].get<double>(), 0.0)
            << "local_classifier.scores[" << axis << "] should be 0.0";
    }
}

// ===========================================================================
// Area and skip_reason fields
// ===========================================================================

TEST(ModerationTraceTest, AreaFieldPresent) {
    auto t = make_trace();
    t.area = "courtroom_2";
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_EQ(j["area"].get<std::string>(), "courtroom_2");
}

TEST(ModerationTraceTest, EmptyAreaIsEmptyString) {
    auto t = make_trace();
    t.area = "";
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_EQ(j["area"].get<std::string>(), "");
}

// ===========================================================================
// Layer2 embedding trace
// ===========================================================================

TEST(ModerationTraceTest, Layer2EmbeddingTrace) {
    auto t = make_trace();
    t.layer2_embedding.ran = true;
    t.layer2_embedding.ns = 9000;
    t.layer2_embedding.dim = 384;

    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_TRUE(j["layers"]["layer2_embedding"]["ran"].get<bool>());
    EXPECT_EQ(j["layers"]["layer2_embedding"]["dim"].get<int>(), 384);
    EXPECT_EQ(j["layers"]["layer2_embedding"]["ns"].get<int64_t>(), 9000);
}

TEST(ModerationTraceTest, Layer2EmbeddingNotRun) {
    auto t = make_trace();
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_FALSE(j["layers"]["layer2_embedding"]["ran"].get<bool>());
    EXPECT_EQ(j["layers"]["layer2_embedding"]["dim"].get<int>(), 0);
}

// ===========================================================================
// Triggered axes and heat values in decision
// ===========================================================================

TEST(ModerationTraceTest, TriggeredAxesEmptyWhenNone) {
    auto t = make_trace();
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_EQ(j["decision"]["triggered_axes"].size(), 0u);
}

TEST(ModerationTraceTest, HeatValuesRoundTrip) {
    auto t = make_trace();
    t.heat_before = 3.14;
    t.heat_delta = 2.72;
    t.heat_after = 5.86;
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_DOUBLE_EQ(j["decision"]["heat_before"].get<double>(), 3.14);
    EXPECT_DOUBLE_EQ(j["decision"]["heat_delta"].get<double>(), 2.72);
    EXPECT_DOUBLE_EQ(j["decision"]["heat_after"].get<double>(), 5.86);
}

// ===========================================================================
// JSON output is valid single-line JSON (no embedded newlines)
// ===========================================================================

TEST(ModerationTraceTest, OutputIsSingleLine) {
    auto t = make_trace();
    t.message = "line1\nline2\ttab";
    auto line = trace_to_json_line(t);
    // JSON string escaping should replace literal newlines with \n
    EXPECT_EQ(line.find('\n'), std::string::npos);
}

// ===========================================================================
// Both keysmash and noheat suppressed simultaneously
// ===========================================================================

TEST(ModerationTraceTest, BothSuppressedFlagsTrue) {
    auto t = make_trace();
    t.keysmash_suppressed = true;
    t.noheat_suppressed = true;
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_TRUE(j["decision"]["keysmash_suppressed"].get<bool>());
    EXPECT_TRUE(j["decision"]["noheat_suppressed"].get<bool>());
}

// ===========================================================================
// Reason string with special characters
// ===========================================================================

TEST(ModerationTraceTest, ReasonWithSpecialChars) {
    auto t = make_trace();
    t.reason = "hate=0.95 (\"high\") + violence=0.3";
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_EQ(j["decision"]["reason"].get<std::string>(), "hate=0.95 (\"high\") + violence=0.3");
}
