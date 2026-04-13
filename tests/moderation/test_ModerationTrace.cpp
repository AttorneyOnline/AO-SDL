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
