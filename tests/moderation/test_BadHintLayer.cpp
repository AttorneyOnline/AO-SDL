#include "moderation/BadHintLayer.h"

#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

using moderation::BadHintConfig;
using moderation::BadHintLayer;
using moderation::EmbeddingBackend;
using moderation::EmbeddingResult;

// A simple deterministic mock backend: returns caller-controlled
// unit vectors for each input text via a lookup table. If no entry
// is registered for a given text, returns a not-ok result. Mirrors
// the pattern in test_SafeHintLayer.cpp.
class MockBackend : public EmbeddingBackend {
  public:
    int dimension() const override {
        return dim_;
    }
    bool is_ready() const override {
        return true;
    }
    EmbeddingResult embed(std::string_view text) override {
        EmbeddingResult r;
        auto it = table_.find(std::string(text));
        if (it == table_.end()) {
            r.error = "no mock entry";
            return r;
        }
        r.ok = true;
        r.vector = it->second;
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

  private:
    int dim_ = 3;
    std::unordered_map<std::string, std::vector<float>> table_;
};

BadHintConfig enabled_cfg(double threshold = 0.75) {
    BadHintConfig c;
    c.enabled = true;
    c.similarity_threshold = threshold;
    c.inject_score = 1.0;
    c.inject_axis = "hate";
    return c;
}

EmbeddingResult make_result(std::vector<float> v) {
    EmbeddingResult r;
    r.ok = true;
    r.vector = std::move(v);
    return r;
}

// Normalize a vector in place to unit length. The contract with
// EmbeddingBackend is that embeddings are L2-normalized — we
// respect that contract in the mock setup so dot-products work
// as cosine similarities.
std::vector<float> unit(std::vector<float> v) {
    double norm2 = 0.0;
    for (float x : v)
        norm2 += static_cast<double>(x) * static_cast<double>(x);
    double n = std::sqrt(norm2);
    if (n <= 0.0)
        return v;
    for (float& x : v)
        x = static_cast<float>(x / n);
    return v;
}

} // namespace

TEST(BadHintLayerTest, InactiveByDefault) {
    BadHintLayer layer;
    EXPECT_FALSE(layer.is_active());
    EXPECT_EQ(layer.anchor_count(), 0u);
}

TEST(BadHintLayerTest, InactiveWithoutAnchors) {
    BadHintLayer layer;
    layer.configure(enabled_cfg());
    EXPECT_FALSE(layer.is_active());
}

TEST(BadHintLayerTest, LoadAnchorsEmbedsAndActivates) {
    MockBackend backend;
    backend.set("bad phrase 1", unit({1.0f, 0.0f, 0.0f}));
    backend.set("bad phrase 2", unit({0.0f, 1.0f, 0.0f}));

    BadHintLayer layer;
    layer.configure(enabled_cfg());
    size_t loaded = layer.load_anchors({"bad phrase 1", "bad phrase 2"}, backend);
    EXPECT_EQ(loaded, 2u);
    EXPECT_TRUE(layer.is_active());
    EXPECT_EQ(layer.anchor_count(), 2u);
}

TEST(BadHintLayerTest, LoadAnchorsSkipsEmptyAndFailed) {
    MockBackend backend;
    backend.set("good anchor", unit({1.0f, 0.0f, 0.0f}));
    // "not in mock" intentionally unset → backend returns not-ok.

    BadHintLayer layer;
    layer.configure(enabled_cfg());
    size_t loaded = layer.load_anchors({"good anchor", "", "not in mock"}, backend);
    EXPECT_EQ(loaded, 1u); // only "good anchor" survives
}

TEST(BadHintLayerTest, HitsWhenSimilarityAboveThreshold) {
    MockBackend backend;
    auto anchor_vec = unit({1.0f, 0.0f, 0.0f});
    backend.set("anchor", anchor_vec);

    BadHintLayer layer;
    layer.configure(enabled_cfg(0.7)); // lower threshold to make assertions easier
    layer.load_anchors({"anchor"}, backend);

    // Query with an embedding very close to the anchor.
    auto message_vec = unit({0.99f, 0.01f, 0.0f}); // cos ≈ 0.9999
    auto r = layer.query_with_embedding(make_result(message_vec));
    EXPECT_TRUE(r.is_bad);
    EXPECT_GT(r.max_similarity, 0.99);
    EXPECT_EQ(r.best_anchor_index, 0);
}

TEST(BadHintLayerTest, MissesWhenSimilarityBelowThreshold) {
    MockBackend backend;
    backend.set("anchor", unit({1.0f, 0.0f, 0.0f}));

    BadHintLayer layer;
    layer.configure(enabled_cfg(0.9));
    layer.load_anchors({"anchor"}, backend);

    // Orthogonal vector → cosine similarity = 0, below threshold.
    auto r = layer.query_with_embedding(make_result(unit({0.0f, 1.0f, 0.0f})));
    EXPECT_FALSE(r.is_bad);
    EXPECT_NEAR(r.max_similarity, 0.0, 1e-6);
}

TEST(BadHintLayerTest, SelectsNearestAnchorWhenMultiple) {
    MockBackend backend;
    backend.set("anchor_a", unit({1.0f, 0.0f, 0.0f}));
    backend.set("anchor_b", unit({0.0f, 1.0f, 0.0f}));
    backend.set("anchor_c", unit({0.0f, 0.0f, 1.0f}));

    BadHintLayer layer;
    layer.configure(enabled_cfg(0.5));
    layer.load_anchors({"anchor_a", "anchor_b", "anchor_c"}, backend);

    // Closest to anchor_b.
    auto r = layer.query_with_embedding(make_result(unit({0.1f, 0.9f, 0.1f})));
    EXPECT_TRUE(r.is_bad);
    EXPECT_EQ(r.best_anchor_index, 1);
}

TEST(BadHintLayerTest, QueryWithNotOkEmbeddingFailsClosed) {
    MockBackend backend;
    backend.set("anchor", unit({1.0f, 0.0f, 0.0f}));

    BadHintLayer layer;
    layer.configure(enabled_cfg());
    layer.load_anchors({"anchor"}, backend);

    EmbeddingResult bad;
    bad.ok = false;
    bad.vector = {1.0f, 0.0f, 0.0f};
    auto r = layer.query_with_embedding(bad);
    EXPECT_FALSE(r.is_bad);
}

TEST(BadHintLayerTest, QueryWithDimMismatchFailsClosed) {
    MockBackend backend;
    backend.set("anchor", unit({1.0f, 0.0f, 0.0f}));

    BadHintLayer layer;
    layer.configure(enabled_cfg());
    layer.load_anchors({"anchor"}, backend);

    // 4-dim vector against 3-dim anchors → dim mismatch → fail closed.
    auto r = layer.query_with_embedding(make_result({1.0f, 0.0f, 0.0f, 0.0f}));
    EXPECT_FALSE(r.is_bad);
}

TEST(BadHintLayerTest, DisabledConfigMakesLayerInactive) {
    MockBackend backend;
    backend.set("anchor", unit({1.0f, 0.0f, 0.0f}));

    BadHintLayer layer;
    auto cfg = enabled_cfg();
    cfg.enabled = false;
    layer.configure(cfg);
    // load_anchors still populates the vectors, but is_active returns
    // false because enabled=false.
    layer.load_anchors({"anchor"}, backend);
    EXPECT_FALSE(layer.is_active());

    // Query also fails-closed when disabled.
    auto r = layer.query_with_embedding(make_result(unit({0.99f, 0.01f, 0.0f})));
    EXPECT_FALSE(r.is_bad);
}

TEST(BadHintLayerTest, InjectConfigIsReadable) {
    BadHintLayer layer;
    auto cfg = enabled_cfg();
    cfg.inject_axis = "violence";
    cfg.inject_score = 0.75;
    layer.configure(cfg);
    EXPECT_EQ(layer.inject_axis(), "violence");
    EXPECT_DOUBLE_EQ(layer.inject_score(), 0.75);
}

TEST(BadHintLayerTest, ReconfigurePreservesLoadedAnchors) {
    // Mirrors SafeHintLayer's policy: scalar reconfig shouldn't
    // clear already-loaded anchors. Operator can bump the threshold
    // without re-embedding.
    MockBackend backend;
    backend.set("anchor", unit({1.0f, 0.0f, 0.0f}));

    BadHintLayer layer;
    layer.configure(enabled_cfg(0.5));
    layer.load_anchors({"anchor"}, backend);
    ASSERT_TRUE(layer.is_active());
    ASSERT_EQ(layer.anchor_count(), 1u);

    // Reconfigure with a new threshold — anchors should survive.
    layer.configure(enabled_cfg(0.99));
    EXPECT_TRUE(layer.is_active());
    EXPECT_EQ(layer.anchor_count(), 1u);

    // A vector that's clearly below 0.99 similarity (cos 45° = 0.707)
    // should NOT hit after the threshold bump. Verifies the new
    // threshold took effect.
    auto r = layer.query_with_embedding(make_result(unit({1.0f, 1.0f, 0.0f})));
    EXPECT_FALSE(r.is_bad);
    EXPECT_NEAR(r.max_similarity, std::sqrt(0.5), 1e-3);
}
