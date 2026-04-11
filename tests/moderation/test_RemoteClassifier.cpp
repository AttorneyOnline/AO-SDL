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
        ++call_count;
        return response_;
    }
    std::string last_body;
    int call_count = 0;

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
    auto transport = std::make_unique<MockTransport>(
        std::make_pair(200, R"({"results":[{"flagged":false,"categories":{},"category_scores":{}}]})"));
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

TEST(RemoteClassifierTest, CacheHitSkipsTransport) {
    // With cache_enabled, a second classify() for the same (normalized)
    // input should NOT call the transport — the verdict should come
    // straight from the cache.
    RemoteClassifier rc;
    auto cfg = active_config();
    cfg.cache_enabled = true;
    cfg.cache_ttl_seconds = 60;
    cfg.cache_max_entries = 10;
    rc.configure(cfg);

    const std::string body = R"({"results":[{"flagged":false,"categories":{},"category_scores":{"hate":0.12}}]})";
    auto transport = std::make_unique<MockTransport>(std::make_pair(200, body));
    auto* transport_ref = transport.get();
    rc.set_transport(std::move(transport));

    auto first = rc.classify("hello there");
    ASSERT_TRUE(first.ok);
    EXPECT_EQ(transport_ref->call_count, 1);
    EXPECT_EQ(rc.cache_misses(), 1);
    EXPECT_EQ(rc.cache_hits(), 0);

    // Same input → cache hit, no new transport call.
    auto second = rc.classify("hello there");
    ASSERT_TRUE(second.ok);
    EXPECT_NEAR(second.scores.hate, 0.12, 1e-6);
    EXPECT_EQ(transport_ref->call_count, 1);
    EXPECT_EQ(rc.cache_hits(), 1);

    // Case / whitespace normalization: still a hit.
    auto third = rc.classify("  HELLO   THERE  ");
    EXPECT_EQ(transport_ref->call_count, 1);
    EXPECT_EQ(rc.cache_hits(), 2);
}

TEST(RemoteClassifierTest, CacheDisabledAlwaysCallsTransport) {
    // Default config has cache_enabled=false. The same input called
    // twice must hit the transport twice — regression guard for the
    // "cache silently introduces state" failure mode.
    RemoteClassifier rc;
    rc.configure(active_config()); // cache_enabled default false
    const std::string body = R"({"results":[{"flagged":false,"categories":{},"category_scores":{}}]})";
    auto transport = std::make_unique<MockTransport>(std::make_pair(200, body));
    auto* transport_ref = transport.get();
    rc.set_transport(std::move(transport));

    rc.classify("hello there");
    rc.classify("hello there");
    rc.classify("hello there");
    EXPECT_EQ(transport_ref->call_count, 3);
    EXPECT_EQ(rc.cache_hits(), 0);
    EXPECT_EQ(rc.cache_misses(), 0);
}

TEST(RemoteClassifierTest, CacheDoesNotStoreErrorResults) {
    // Transient failures must not be cached — the retry path would
    // otherwise get pinned to an error for the TTL window.
    RemoteClassifier rc;
    auto cfg = active_config();
    cfg.cache_enabled = true;
    rc.configure(cfg);
    auto transport = std::make_unique<MockTransport>(std::make_pair(500, R"({"error":"flaky"})"));
    auto* transport_ref = transport.get();
    rc.set_transport(std::move(transport));

    auto first = rc.classify("hello there");
    EXPECT_FALSE(first.ok);
    EXPECT_EQ(transport_ref->call_count, 1);

    // Second call must still hit the transport.
    auto second = rc.classify("hello there");
    EXPECT_FALSE(second.ok);
    EXPECT_EQ(transport_ref->call_count, 2);
}

TEST(RemoteClassifierTest, CacheDistinguishesDifferentMessages) {
    RemoteClassifier rc;
    auto cfg = active_config();
    cfg.cache_enabled = true;
    rc.configure(cfg);
    const std::string body = R"({"results":[{"flagged":false,"categories":{},"category_scores":{}}]})";
    auto transport = std::make_unique<MockTransport>(std::make_pair(200, body));
    auto* transport_ref = transport.get();
    rc.set_transport(std::move(transport));

    rc.classify("message alpha");
    rc.classify("message bravo");
    rc.classify("message charlie");
    EXPECT_EQ(transport_ref->call_count, 3);
    EXPECT_EQ(rc.cache_hits(), 0);
    EXPECT_EQ(rc.cache_misses(), 3);
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
