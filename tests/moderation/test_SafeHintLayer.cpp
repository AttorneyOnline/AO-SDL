#include "moderation/SafeHintLayer.h"

#include <cmath>
#include <gtest/gtest.h>
#include <string>
#include <unordered_map>

namespace {

using moderation::EmbeddingBackend;
using moderation::EmbeddingResult;
using moderation::SafeHintConfig;
using moderation::SafeHintLayer;

// Deterministic embedding backend for tests. Maps each registered
// string to a hand-picked unit vector so the dot products are
// predictable. Unknown strings get a uniform-random-ish vector
// (derived from the string hash) so the cosine against any anchor
// is low.
//
// Using a real embedding model in tests would pull in llama.cpp
// + download a GGUF + need a several-MB fixture — none of which
// exercises SafeHintLayer's actual logic. A mock is the right
// tradeoff here.
class MockBackend : public EmbeddingBackend {
  public:
    static constexpr int kDim = 4;

    MockBackend() {
        // Four orthogonal unit vectors + one diagonal.
        table_["hello"] = {1.0f, 0.0f, 0.0f, 0.0f};
        table_["world"] = {0.0f, 1.0f, 0.0f, 0.0f};
        table_["foo"] = {0.0f, 0.0f, 1.0f, 0.0f};
        table_["bar"] = {0.0f, 0.0f, 0.0f, 1.0f};
        // Very close to "hello" — dot product 0.98058...
        table_["hello there"] = {0.98058068f, 0.19611614f, 0.0f, 0.0f};
        // 45 degrees between "hello" and "world".
        table_["between"] = {0.70710678f, 0.70710678f, 0.0f, 0.0f};
    }

    int dimension() const override {
        return kDim;
    }
    bool is_ready() const override {
        return ready_;
    }

    EmbeddingResult embed(std::string_view text) override {
        EmbeddingResult r;
        if (!ready_) {
            r.error = "not_ready";
            return r;
        }
        if (text.empty()) {
            r.error = "empty";
            return r;
        }
        std::string key(text);
        auto it = table_.find(key);
        if (it != table_.end()) {
            r.ok = true;
            r.vector = it->second;
            r.token_count = 1;
            return r;
        }
        // Unknown text: produce a junk unit vector from the string
        // hash so the cosine against any registered anchor is low.
        // Guarantees reproducibility across runs (hash is pure).
        std::hash<std::string> h;
        auto seed = static_cast<uint32_t>(h(key));
        float v[kDim];
        double sum_sq = 0.0;
        for (int i = 0; i < kDim; ++i) {
            // Step the seed through a cheap xorshift and pull a
            // float in [-1, 1]. This produces vectors that are
            // essentially orthogonal to the hand-crafted ones.
            seed ^= seed << 13;
            seed ^= seed >> 17;
            seed ^= seed << 5;
            v[i] = static_cast<float>((seed & 0xFFFF) / 32768.0 - 1.0);
            sum_sq += v[i] * v[i];
        }
        auto norm = static_cast<float>(std::sqrt(sum_sq));
        if (norm == 0.0f)
            norm = 1.0f;
        r.vector.resize(kDim);
        for (int i = 0; i < kDim; ++i)
            r.vector[i] = v[i] / norm;
        r.ok = true;
        r.token_count = 1;
        return r;
    }

    const char* name() const override {
        return "mock";
    }

    void set_ready(bool r) {
        ready_ = r;
    }

  private:
    bool ready_ = true;
    std::unordered_map<std::string, std::vector<float>> table_;
};

SafeHintConfig make_cfg(double threshold = 0.7) {
    SafeHintConfig cfg;
    cfg.enabled = true;
    cfg.similarity_threshold = threshold;
    return cfg;
}

} // namespace

TEST(SafeHintLayer, InertBeforeConfigure) {
    SafeHintLayer layer;
    EXPECT_FALSE(layer.is_active());
    EXPECT_EQ(layer.anchor_count(), 0u);
}

TEST(SafeHintLayer, InertWithoutAnchors) {
    SafeHintLayer layer;
    layer.configure(make_cfg());
    // Config enabled, but zero anchors loaded.
    EXPECT_FALSE(layer.is_active());
}

TEST(SafeHintLayer, LoadsAnchorsSuccessfully) {
    SafeHintLayer layer;
    layer.configure(make_cfg());
    MockBackend backend;
    size_t loaded = layer.load_anchors({"hello", "world", "foo"}, backend);
    EXPECT_EQ(loaded, 3u);
    EXPECT_EQ(layer.anchor_count(), 3u);
    EXPECT_TRUE(layer.is_active());
}

TEST(SafeHintLayer, SkipsAnchorsWhenBackendNotReady) {
    SafeHintLayer layer;
    layer.configure(make_cfg());
    MockBackend backend;
    backend.set_ready(false);
    size_t loaded = layer.load_anchors({"hello", "world"}, backend);
    EXPECT_EQ(loaded, 0u);
    EXPECT_FALSE(layer.is_active());
}

TEST(SafeHintLayer, QueryExactMatchIsSafe) {
    SafeHintLayer layer;
    layer.configure(make_cfg(0.7));
    MockBackend backend;
    layer.load_anchors({"hello", "world"}, backend);

    auto r = layer.query("hello", backend);
    EXPECT_TRUE(r.is_safe);
    EXPECT_NEAR(r.max_similarity, 1.0, 1e-6);
    EXPECT_EQ(r.best_anchor_index, 0);
}

TEST(SafeHintLayer, QueryNearMatchIsSafe) {
    SafeHintLayer layer;
    layer.configure(make_cfg(0.7));
    MockBackend backend;
    layer.load_anchors({"hello"}, backend);

    // "hello there" was hand-crafted to have dot 0.98 with "hello"
    auto r = layer.query("hello there", backend);
    EXPECT_TRUE(r.is_safe);
    EXPECT_GT(r.max_similarity, 0.95);
    EXPECT_EQ(r.best_anchor_index, 0);
}

TEST(SafeHintLayer, QueryOrthogonalNotSafe) {
    SafeHintLayer layer;
    layer.configure(make_cfg(0.7));
    MockBackend backend;
    layer.load_anchors({"hello"}, backend);

    // "world" is orthogonal to "hello" — dot product is 0.
    auto r = layer.query("world", backend);
    EXPECT_FALSE(r.is_safe);
    EXPECT_NEAR(r.max_similarity, 0.0, 1e-6);
}

TEST(SafeHintLayer, QueryRespectsThreshold) {
    SafeHintLayer layer;
    MockBackend backend;

    // "between" has cosine 0.707 with both "hello" and "world".
    // With threshold 0.7, it's safe. With threshold 0.8, it's not.
    {
        layer.configure(make_cfg(0.7));
        layer.load_anchors({"hello", "world"}, backend);
        auto r = layer.query("between", backend);
        EXPECT_TRUE(r.is_safe);
        EXPECT_NEAR(r.max_similarity, 0.7071, 1e-3);
    }
    {
        layer.configure(make_cfg(0.8));
        layer.load_anchors({"hello", "world"}, backend);
        auto r = layer.query("between", backend);
        EXPECT_FALSE(r.is_safe);
    }
}

TEST(SafeHintLayer, QueryPicksBestAnchor) {
    SafeHintLayer layer;
    layer.configure(make_cfg(0.5));
    MockBackend backend;
    // Anchor order: hello, foo, world.
    layer.load_anchors({"hello", "foo", "world"}, backend);

    // "hello there" is closest to anchor 0 ("hello").
    auto r = layer.query("hello there", backend);
    EXPECT_TRUE(r.is_safe);
    EXPECT_EQ(r.best_anchor_index, 0);

    // "world" exactly = anchor 2.
    auto r2 = layer.query("world", backend);
    EXPECT_TRUE(r2.is_safe);
    EXPECT_EQ(r2.best_anchor_index, 2);
}

TEST(SafeHintLayer, QueryEmptyMessageReturnsUnsafe) {
    SafeHintLayer layer;
    layer.configure(make_cfg());
    MockBackend backend;
    layer.load_anchors({"hello"}, backend);

    auto r = layer.query("", backend);
    EXPECT_FALSE(r.is_safe);
    EXPECT_EQ(r.max_similarity, 0.0);
}

TEST(SafeHintLayer, QueryWithBackendNotReady) {
    SafeHintLayer layer;
    layer.configure(make_cfg());
    MockBackend backend;
    layer.load_anchors({"hello"}, backend);
    // Anchors were loaded while backend was ready; now pretend the
    // backend went away. query() should return unsafe, not crash.
    backend.set_ready(false);
    auto r = layer.query("hello", backend);
    EXPECT_FALSE(r.is_safe);
    EXPECT_EQ(r.max_similarity, 0.0);
}

TEST(SafeHintLayer, DisabledByConfig) {
    SafeHintLayer layer;
    SafeHintConfig cfg = make_cfg();
    cfg.enabled = false;
    layer.configure(cfg);
    MockBackend backend;
    layer.load_anchors({"hello"}, backend);
    EXPECT_FALSE(layer.is_active());

    // Even with anchors loaded, a disabled layer reports every
    // message as unsafe (which translates to "don't shortcut,
    // fall through to the remote classifier").
    auto r = layer.query("hello", backend);
    EXPECT_FALSE(r.is_safe);
}

TEST(SafeHintLayer, LoadedAnchorCountExcludesFailures) {
    SafeHintLayer layer;
    layer.configure(make_cfg());
    MockBackend backend;
    // Empty strings should be silently dropped, not counted.
    size_t loaded = layer.load_anchors({"hello", "", "world", ""}, backend);
    EXPECT_EQ(loaded, 2u);
    EXPECT_EQ(layer.anchor_count(), 2u);
}
