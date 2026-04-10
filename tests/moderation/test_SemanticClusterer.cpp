#include "moderation/EmbeddingBackend.h"
#include "moderation/SemanticClusterer.h"

#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <unordered_map>

namespace {

using moderation::EmbeddingBackend;
using moderation::EmbeddingResult;
using moderation::EmbeddingsLayerConfig;
using moderation::SemanticClusterer;

/// Deterministic test backend that maps exact message strings to
/// hand-chosen vectors. Lets us drive the clustering math without
/// actually running a model.
class StubBackend : public EmbeddingBackend {
  public:
    void set(const std::string& text, std::vector<float> v) {
        // L2-normalize so the caller doesn't have to.
        double n = 0;
        for (float x : v)
            n += static_cast<double>(x) * static_cast<double>(x);
        if (n > 0) {
            float inv = static_cast<float>(1.0 / std::sqrt(n));
            for (float& x : v)
                x *= inv;
        }
        map_[text] = std::move(v);
    }

    int dimension() const override {
        return 3;
    }
    bool is_ready() const override {
        return true;
    }
    EmbeddingResult embed(std::string_view text) override {
        EmbeddingResult r;
        auto it = map_.find(std::string(text));
        if (it == map_.end()) {
            r.error = "unknown test input";
            return r;
        }
        r.ok = true;
        r.vector = it->second;
        return r;
    }
    const char* name() const override {
        return "stub";
    }

  private:
    std::unordered_map<std::string, std::vector<float>> map_;
};

int64_t g_clock_ms = 1'000'000'000LL;
int64_t fake_clock() {
    return g_clock_ms;
}

EmbeddingsLayerConfig test_cfg() {
    EmbeddingsLayerConfig c;
    c.enabled = true;
    c.hf_model_id = "stub";
    c.ring_size = 20;
    c.similarity_threshold = 0.9;
    c.cluster_threshold = 3;
    c.window_seconds = 60;
    return c;
}

class SemanticClustererTest : public ::testing::Test {
  protected:
    void SetUp() override {
        g_clock_ms = 1'000'000'000LL;
        auto stub = std::make_unique<StubBackend>();
        stub_ = stub.get();
        clusterer_.set_clock_for_tests(fake_clock);
        clusterer_.configure(test_cfg());
        clusterer_.set_backend(std::move(stub));
    }

    SemanticClusterer clusterer_;
    StubBackend* stub_ = nullptr;
};

} // namespace

TEST_F(SemanticClustererTest, DisabledLayerScoresZero) {
    EmbeddingsLayerConfig cfg = test_cfg();
    cfg.enabled = false;
    clusterer_.configure(cfg);
    stub_->set("anything", {1.0f, 0.0f, 0.0f});
    auto r = clusterer_.score("ipid1", "anything");
    EXPECT_EQ(r.score, 0.0);
}

TEST_F(SemanticClustererTest, SingleMessageDoesNotCluster) {
    stub_->set("lonely", {1.0f, 0.0f, 0.0f});
    auto r = clusterer_.score("ipid1", "lonely");
    EXPECT_EQ(r.score, 0.0);
    EXPECT_EQ(r.distinct_ipids, 0);
}

TEST_F(SemanticClustererTest, TwoIdenticalMessagesSameIpidDoNotCluster) {
    stub_->set("hello", {1.0f, 0.0f, 0.0f});
    clusterer_.score("ipid1", "hello");
    auto r = clusterer_.score("ipid1", "hello");
    // Same IPID — we require distinct IPIDs to cross the cluster
    // threshold. "ipid1" appears once before in the ring, but it's
    // not "distinct" from the current caller.
    EXPECT_EQ(r.score, 0.0);
}

TEST_F(SemanticClustererTest, ThreeDistinctIpidsCrossThreshold) {
    stub_->set("spam", {1.0f, 0.0f, 0.0f});
    EXPECT_EQ(clusterer_.score("ipid1", "spam").score, 0.0);
    EXPECT_EQ(clusterer_.score("ipid2", "spam").score, 0.0);
    // The third distinct IPID pushes distinct_others from 2 to 2
    // (the caller is excluded), and distinct_others+1 = 3 ≥ threshold.
    auto r = clusterer_.score("ipid3", "spam");
    EXPECT_GT(r.score, 0.0);
    EXPECT_EQ(r.distinct_ipids, 2);
}

TEST_F(SemanticClustererTest, NearDuplicateStillClusters) {
    stub_->set("a", {1.0f, 0.0f, 0.0f});
    stub_->set("a2", {0.99f, 0.14f, 0.0f}); // cosine ~0.99 with "a"
    stub_->set("a3", {0.98f, 0.20f, 0.0f}); // cosine ~0.98 with "a"
    clusterer_.score("ipid1", "a");
    clusterer_.score("ipid2", "a2");
    auto r = clusterer_.score("ipid3", "a3");
    EXPECT_GT(r.score, 0.0);
}

TEST_F(SemanticClustererTest, DissimilarMessagesDoNotCluster) {
    stub_->set("a", {1.0f, 0.0f, 0.0f});
    stub_->set("b", {0.0f, 1.0f, 0.0f});
    stub_->set("c", {0.0f, 0.0f, 1.0f});
    clusterer_.score("ipid1", "a");
    clusterer_.score("ipid2", "b");
    auto r = clusterer_.score("ipid3", "c");
    EXPECT_EQ(r.score, 0.0);
}

TEST_F(SemanticClustererTest, ExpiredEntriesPruned) {
    stub_->set("same", {1.0f, 0.0f, 0.0f});
    clusterer_.score("ipid1", "same");
    clusterer_.score("ipid2", "same");

    // Advance past the window.
    g_clock_ms += 120LL * 1000; // 120s > 60s window
    clusterer_.sweep();

    // Prior messages are gone — a new message from ipid3 sees an
    // empty window and does not cluster.
    auto r = clusterer_.score("ipid3", "same");
    EXPECT_EQ(r.score, 0.0);
    EXPECT_EQ(r.distinct_ipids, 0);
}

TEST_F(SemanticClustererTest, NullBackendReturnsZero) {
    SemanticClusterer c;
    c.configure(test_cfg());
    c.set_backend(std::make_unique<moderation::NullEmbeddingBackend>());
    auto r = c.score("ipid1", "anything");
    EXPECT_EQ(r.score, 0.0);
}
