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
    // An all-default trace should still serialize without throwing
    // and parse back as valid JSON. Every layer should have
    // `ran: false` by default.
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
    EXPECT_FALSE(j["layers"]["safe_hint"]["ran"].get<bool>());
    EXPECT_FALSE(j["layers"]["remote"]["ran"].get<bool>());
    EXPECT_EQ(j["decision"]["skip_reason"].get<std::string>(), "");
    EXPECT_EQ(j["decision"]["final_action"].get<std::string>(), "NONE");
}

TEST(ModerationTraceTest, PopulatedLayersRoundTrip) {
    auto t = make_trace();
    // Unicode classifier flagged something.
    t.unicode.ran = true;
    t.unicode.ns = 120;
    t.unicode.visual_noise = 0.35;

    // URLs found an allowlisted match.
    t.urls.ran = true;
    t.urls.ns = 80;
    t.urls.link_risk = 0.0;
    t.urls.urls = {"https://example.com", "http://wiki.example.org"};

    // Slur filter fired on a match.
    t.slurs.ran = true;
    t.slurs.ns = 340;
    t.slurs.match_score = 1.0;
    t.slurs.matches = {"slur_token"};

    // Local classifier ran, max_confidence 0.87 on the hate axis.
    t.local_classifier.ran = true;
    t.local_classifier.ns = 210;
    t.local_classifier.max_confidence = 0.87;
    t.local_classifier.max_category_index = 1;
    t.local_classifier.scores.hate = 0.87;
    t.local_classifier.scores.toxicity = 0.65;

    // Bad-hint layer matched.
    t.bad_hint.ran = true;
    t.bad_hint.ns = 410;
    t.bad_hint.max_similarity = 0.82;
    t.bad_hint.best_anchor_index = 7;
    t.bad_hint.is_bad = true;

    // Decision
    t.skip_reason = "local_classifier_bad";
    t.triggered_axes = {"toxicity_lc", "hate_lc"};
    t.final_scores.hate = 0.87;
    t.final_scores.toxicity = 0.65;
    t.heat_before = 0.0;
    t.heat_delta = 6.0;
    t.heat_after = 6.0;
    t.final_action = ModerationAction::MUTE;
    t.reason = "hate=0.87,toxicity=0.65";

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
    EXPECT_EQ(j["decision"]["skip_reason"].get<std::string>(), "local_classifier_bad");
    EXPECT_EQ(j["decision"]["final_action"].get<std::string>(), "MUTE");
    EXPECT_EQ(j["decision"]["triggered_axes"].size(), 2u);
    EXPECT_DOUBLE_EQ(j["decision"]["heat_delta"].get<double>(), 6.0);
    EXPECT_DOUBLE_EQ(j["decision"]["heat_after"].get<double>(), 6.0);
}

TEST(ModerationTraceTest, RemoteLayerSerialization) {
    auto t = make_trace();
    t.remote.ran = true;
    t.remote.ns = 450000;
    t.remote.http_status = 200;
    t.remote.cache_hit = false;
    t.remote.ok = true;
    t.remote.scores.hate = 0.12;
    t.remote.error = "";

    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_TRUE(j["layers"]["remote"]["ran"].get<bool>());
    EXPECT_EQ(j["layers"]["remote"]["http_status"].get<int>(), 200);
    EXPECT_FALSE(j["layers"]["remote"]["cache_hit"].get<bool>());
    EXPECT_TRUE(j["layers"]["remote"]["ok"].get<bool>());
    EXPECT_DOUBLE_EQ(j["layers"]["remote"]["scores"]["hate"].get<double>(), 0.12);
}

TEST(ModerationTraceTest, RemoteCacheHitSerialization) {
    // When a remote cache hit occurred, cache_hit is true and ns
    // should reflect the short cache-lookup cost, not the HTTP call.
    auto t = make_trace();
    t.remote.ran = true;
    t.remote.ns = 12000; // ~12 µs for a cache lookup
    t.remote.http_status = 200;
    t.remote.cache_hit = true;
    t.remote.ok = true;

    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_TRUE(j["layers"]["remote"]["cache_hit"].get<bool>());
    EXPECT_LT(j["layers"]["remote"]["ns"].get<int64_t>(), 100000);
}

TEST(ModerationTraceTest, TrustBankSkipped) {
    // A trust_bank skip is represented with ran=true AND skipped=true,
    // plus the current_heat that drove the decision. Grafana queries
    // can correlate per-IPID trust accumulation with skip rate over
    // time using this shape.
    auto t = make_trace();
    t.trust_bank.ran = true;
    t.trust_bank.current_heat = -4.2;
    t.trust_bank.skip_rate = 0.85;
    t.trust_bank.skipped = true;
    t.skip_reason = "trust_bank";

    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    EXPECT_TRUE(j["layers"]["trust_bank"]["ran"].get<bool>());
    EXPECT_TRUE(j["layers"]["trust_bank"]["skipped"].get<bool>());
    EXPECT_DOUBLE_EQ(j["layers"]["trust_bank"]["current_heat"].get<double>(), -4.2);
    EXPECT_EQ(j["decision"]["skip_reason"].get<std::string>(), "trust_bank");
}

TEST(ModerationTraceTest, ActionStringifies) {
    // Ensure every ModerationAction serializes to a distinct
    // human-readable string. Grafana filters on the decision.final_action
    // label, so stable stringification is load-bearing.
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
    // The scores object MUST include every axis even when unset —
    // LogQL queries on e.g. scores.hate break on missing fields.
    auto t = make_trace();
    auto line = trace_to_json_line(t);
    auto j = nlohmann::json::parse(line);
    const char* axes[] = {"visual_noise", "link_risk",     "slurs",    "toxicity",  "hate",
                          "sexual",       "sexual_minors", "violence", "self_harm", "semantic_echo"};
    for (const char* axis : axes) {
        EXPECT_TRUE(j["decision"]["final_scores"].contains(axis)) << "final_scores missing axis: " << axis;
        EXPECT_TRUE(j["layers"]["local_classifier"]["scores"].contains(axis))
            << "local_classifier.scores missing axis: " << axis;
        EXPECT_TRUE(j["layers"]["remote"]["scores"].contains(axis)) << "remote.scores missing axis: " << axis;
    }
}
