#include "moderation/RemoteClassifier.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

namespace {

using moderation::ModerationAxisScores;
using moderation::parse_openai_response;
using moderation::RemoteClassifier;
using moderation::RemoteClassifierConfig;
using moderation::RemoteClassifierTransport;

class MockTransport : public RemoteClassifierTransport {
  public:
    explicit MockTransport(std::pair<int, std::string> response) : response_(std::move(response)) {
    }
    std::pair<int, std::string> post_json(const std::string& /*url*/, const std::string& /*bearer*/,
                                          const std::string& body, int /*timeout_ms*/) override {
        last_body = body;
        return response_;
    }
    std::string last_body;

  private:
    std::pair<int, std::string> response_;
};

RemoteClassifierConfig active_config() {
    RemoteClassifierConfig c;
    c.enabled = true;
    c.api_key = "sk-test";
    c.model = "omni-moderation-latest";
    c.endpoint = "https://api.openai.com/v1/moderations";
    c.timeout_ms = 500;
    c.fail_open = true;
    return c;
}

} // namespace

TEST(RemoteClassifierTest, DisabledLayerIsInactive) {
    RemoteClassifier rc;
    RemoteClassifierConfig cfg;
    cfg.enabled = false;
    cfg.api_key = "sk-test"; // key alone does not activate
    rc.configure(cfg);
    EXPECT_FALSE(rc.is_active());
}

TEST(RemoteClassifierTest, EmptyKeyIsInactive) {
    RemoteClassifier rc;
    RemoteClassifierConfig cfg;
    cfg.enabled = true; // enabled alone does not activate
    rc.configure(cfg);
    EXPECT_FALSE(rc.is_active());
}

TEST(RemoteClassifierTest, SuccessfulResponseMapsScores) {
    RemoteClassifier rc;
    rc.configure(active_config());
    const std::string body = R"({
        "id": "modr-1",
        "model": "omni-moderation-latest",
        "results": [{
            "flagged": true,
            "categories": {"hate": true, "sexual": false},
            "category_scores": {
                "hate": 0.87,
                "hate/threatening": 0.4,
                "harassment": 0.6,
                "sexual": 0.02,
                "sexual/minors": 0.0,
                "violence": 0.1,
                "violence/graphic": 0.05,
                "self-harm": 0.0,
                "self-harm/intent": 0.0,
                "self-harm/instructions": 0.0,
                "illicit": 0.3,
                "illicit/violent": 0.1
            }
        }]
    })";
    rc.set_transport(std::make_unique<MockTransport>(std::make_pair(200, body)));

    auto result = rc.classify("slur-bearing message");
    ASSERT_TRUE(result.ok);
    EXPECT_NEAR(result.scores.hate, 0.87, 1e-6);
    // toxicity is max over harassment/harassment_threat/illicit/illicit_violent
    EXPECT_NEAR(result.scores.toxicity, 0.6, 1e-6);
    EXPECT_NEAR(result.scores.violence, 0.1, 1e-6);
    EXPECT_NEAR(result.scores.sexual_minors, 0.0, 1e-6);
}

TEST(RemoteClassifierTest, HttpErrorReturnsNotOk) {
    RemoteClassifier rc;
    rc.configure(active_config());
    rc.set_transport(std::make_unique<MockTransport>(std::make_pair(500, R"({"error": "rate_limited"})")));

    auto result = rc.classify("anything");
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.http_status, 500);
    EXPECT_NE(result.error.find("http 500"), std::string::npos);
}

TEST(RemoteClassifierTest, TransportFailureReturnsNotOk) {
    RemoteClassifier rc;
    rc.configure(active_config());
    rc.set_transport(std::make_unique<MockTransport>(std::make_pair(0, "connection refused")));

    auto result = rc.classify("anything");
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.http_status, 0);
    EXPECT_NE(result.error.find("connection refused"), std::string::npos);
}

TEST(RemoteClassifierTest, BodyContainsModelAndInput) {
    RemoteClassifier rc;
    rc.configure(active_config());
    auto transport = std::make_unique<MockTransport>(std::make_pair(200,
        R"({"results":[{"flagged":false,"categories":{},"category_scores":{}}]})"));
    auto* transport_ref = transport.get();
    rc.set_transport(std::move(transport));

    rc.classify("hello there");
    EXPECT_NE(transport_ref->last_body.find("\"model\":\"omni-moderation-latest\""), std::string::npos);
    EXPECT_NE(transport_ref->last_body.find("\"input\":\"hello there\""), std::string::npos);
}

TEST(RemoteClassifierTest, ParseOpenaiResponseRejectsMalformed) {
    ModerationAxisScores scores;
    std::string err;
    EXPECT_FALSE(parse_openai_response("not json at all", scores, err));
    EXPECT_FALSE(err.empty());
}

TEST(RemoteClassifierTest, ParseOpenaiResponseRejectsMissingResults) {
    ModerationAxisScores scores;
    std::string err;
    EXPECT_FALSE(parse_openai_response(R"({"id":"x","model":"y"})", scores, err));
}

TEST(RemoteClassifierTest, SexualMinorsAxisSeparated) {
    // This is the axis that carries a catastrophic heat weight, so
    // parsing it separately from regular "sexual" is critical.
    ModerationAxisScores scores;
    std::string err;
    const std::string body = R"({
        "results":[{
            "flagged":true,
            "categories":{"sexual/minors":true},
            "category_scores":{"sexual":0.1,"sexual/minors":0.9}
        }]
    })";
    ASSERT_TRUE(parse_openai_response(body, scores, err));
    EXPECT_DOUBLE_EQ(scores.sexual, 0.1);
    EXPECT_DOUBLE_EQ(scores.sexual_minors, 0.9);
}
