#include "moderation/LocalClassifierLayer.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using moderation::EmbeddingResult;
using moderation::LocalClassifierConfig;
using moderation::LocalClassifierLayer;

// Build a valid weights blob in memory with caller-controlled
// weight / bias values. The layout matches the binary format
// documented in LocalClassifierLayer.h.
//
// num_categories is fixed at 8 (kagami's axis count). embedding_dim
// and model_name are caller-controlled so specific tests can assert
// the loader rejects mismatches.
std::vector<uint8_t> build_blob(uint32_t num_cat, uint32_t dim, const std::string& model_name,
                                const std::vector<float>& weights, const std::vector<float>& biases) {
    std::vector<uint8_t> blob;
    // magic
    const char magic[] = "KGCLF";
    blob.insert(blob.end(), magic, magic + 5);
    blob.push_back(0x01);
    blob.push_back(0x00);
    blob.push_back(0x00);
    // uint32 fields
    auto push_u32 = [&](uint32_t v) {
        blob.push_back(static_cast<uint8_t>(v & 0xff));
        blob.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
        blob.push_back(static_cast<uint8_t>((v >> 16) & 0xff));
        blob.push_back(static_cast<uint8_t>((v >> 24) & 0xff));
    };
    push_u32(1); // format version
    push_u32(num_cat);
    push_u32(dim);
    push_u32(static_cast<uint32_t>(model_name.size()));
    blob.insert(blob.end(), model_name.begin(), model_name.end());
    const uint8_t* w_bytes = reinterpret_cast<const uint8_t*>(weights.data());
    blob.insert(blob.end(), w_bytes, w_bytes + weights.size() * sizeof(float));
    const uint8_t* b_bytes = reinterpret_cast<const uint8_t*>(biases.data());
    blob.insert(blob.end(), b_bytes, b_bytes + biases.size() * sizeof(float));
    return blob;
}

// A minimal 3-dim × 8-cat blob with all-zero weights and biases.
// Used by tests that just need a valid-but-inert loaded state.
std::vector<uint8_t> zero_blob(const std::string& model_name = "test-model") {
    constexpr uint32_t num_cat = 8;
    constexpr uint32_t dim = 3;
    std::vector<float> w(num_cat * dim, 0.0f);
    std::vector<float> b(num_cat, 0.0f);
    return build_blob(num_cat, dim, model_name, w, b);
}

LocalClassifierConfig enabled_cfg() {
    LocalClassifierConfig c;
    c.enabled = true;
    c.confidence_high_skip = 0.9;
    c.confidence_low_clean = 0.2;
    return c;
}

EmbeddingResult make_embedding(std::vector<float> vec) {
    EmbeddingResult e;
    e.ok = true;
    e.vector = std::move(vec);
    return e;
}

} // namespace

TEST(LocalClassifierLayerTest, InactiveByDefault) {
    LocalClassifierLayer layer;
    EXPECT_FALSE(layer.is_active());
    EXPECT_EQ(layer.num_categories(), 0);
    EXPECT_EQ(layer.embedding_dim(), 0);
}

TEST(LocalClassifierLayerTest, EnabledButNoWeightsIsInactive) {
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    EXPECT_FALSE(layer.is_active());
}

TEST(LocalClassifierLayerTest, LoadsValidBlob) {
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    auto blob = zero_blob("test-model");
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), "test-model"));
    EXPECT_TRUE(layer.is_active());
    EXPECT_EQ(layer.num_categories(), 8);
    EXPECT_EQ(layer.embedding_dim(), 3);
    EXPECT_EQ(layer.model_name(), "test-model");
}

TEST(LocalClassifierLayerTest, RejectsNullBlob) {
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    EXPECT_FALSE(layer.load_weights(nullptr, 128, "test-model"));
    EXPECT_FALSE(layer.is_active());
}

TEST(LocalClassifierLayerTest, RejectsTruncatedHeader) {
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    std::vector<uint8_t> blob(16, 0); // smaller than kHeaderFixedSize
    EXPECT_FALSE(layer.load_weights(blob.data(), blob.size(), "test-model"));
}

TEST(LocalClassifierLayerTest, RejectsBadMagic) {
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    auto blob = zero_blob();
    blob[0] = 'X'; // corrupt magic
    EXPECT_FALSE(layer.load_weights(blob.data(), blob.size(), "test-model"));
}

TEST(LocalClassifierLayerTest, RejectsWrongFormatVersion) {
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    auto blob = zero_blob();
    blob[8] = 0x99; // version field byte 0
    EXPECT_FALSE(layer.load_weights(blob.data(), blob.size(), "test-model"));
}

TEST(LocalClassifierLayerTest, RejectsSizeMismatch) {
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    auto blob = zero_blob();
    blob.resize(blob.size() - 4); // chop off trailing bytes
    EXPECT_FALSE(layer.load_weights(blob.data(), blob.size(), "test-model"));
}

TEST(LocalClassifierLayerTest, RejectsModelNameMismatchAndDisables) {
    // The compatibility gate: weights file says "model-a", runtime
    // says "model-b" → layer disables. This is the load-bearing
    // safety check that prevents silently wrong outputs against a
    // different embedding model.
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    auto blob = zero_blob("model-a");
    EXPECT_FALSE(layer.load_weights(blob.data(), blob.size(), "model-b"));
    EXPECT_FALSE(layer.is_active());
}

TEST(LocalClassifierLayerTest, AcceptsRuntimeNameWithFilenameSuffix) {
    // kagami's HfModelFetcher pins GGUF quantizations via a
    // "repo:filename.gguf" format. The weights header only stores
    // the repo portion; the loader strips the suffix on both sides
    // before comparing. A repo-match with a different filename
    // MUST load successfully, otherwise every quantization change
    // would force a retrain.
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    auto blob = zero_blob("hf-user/some-model");
    EXPECT_TRUE(layer.load_weights(blob.data(), blob.size(), "hf-user/some-model:q8_0.gguf"));
    EXPECT_TRUE(layer.is_active());
}

TEST(LocalClassifierLayerTest, RejectsDifferentRepoEvenWithSameFilename) {
    // Prefix-match is on the repo portion, NOT the filename. A
    // different repo with the same filename must still be rejected.
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    auto blob = zero_blob("hf-user/model-a");
    EXPECT_FALSE(layer.load_weights(blob.data(), blob.size(), "hf-user/model-b:q8_0.gguf"));
    EXPECT_FALSE(layer.is_active());
}

TEST(LocalClassifierLayerTest, EmptyRuntimeModelNameSkipsCheck) {
    // Tests may pass "" for the runtime model name — that skips
    // the model-name compatibility check entirely. Not a prod path
    // (kagami always passes a non-empty name) but useful for loader
    // unit tests that don't care about the name.
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    auto blob = zero_blob("model-x");
    EXPECT_TRUE(layer.load_weights(blob.data(), blob.size(), ""));
    EXPECT_TRUE(layer.is_active());
}

TEST(LocalClassifierLayerTest, RejectsOutOfRangeNumCategories) {
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    auto blob = zero_blob();
    // Overwrite num_categories (offset 12) with a huge value.
    blob[12] = 0xff;
    blob[13] = 0xff;
    blob[14] = 0xff;
    blob[15] = 0xff;
    EXPECT_FALSE(layer.load_weights(blob.data(), blob.size(), "test-model"));
}

TEST(LocalClassifierLayerTest, ClassifyOnInactiveReturnsRanFalse) {
    LocalClassifierLayer layer;
    // Don't configure / load. Must return ran=false.
    auto r = layer.classify(make_embedding({0.1f, 0.2f, 0.3f}));
    EXPECT_FALSE(r.ran);
}

TEST(LocalClassifierLayerTest, ClassifyProducesMonotonicOutput) {
    // Hand-crafted weights where category 0's row is [1, 0, 0] and
    // bias 0, all other rows zero. Embedding [0.99, 0.01, 0] should
    // make category 0 the argmax.
    constexpr uint32_t num_cat = 8;
    constexpr uint32_t dim = 3;
    std::vector<float> w(num_cat * dim, 0.0f);
    w[0] = 5.0f; // row 0, col 0 — large weight so sigmoid(4.95) ~ 0.993
    std::vector<float> b(num_cat, 0.0f);
    auto blob = build_blob(num_cat, dim, "test-model", w, b);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), "test-model"));

    auto r = layer.classify(make_embedding({0.99f, 0.01f, 0.0f}));
    EXPECT_TRUE(r.ran);
    EXPECT_EQ(r.max_category_index, 0);
    // Logit = 5.0 * 0.99 = 4.95, sigmoid(4.95) ≈ 0.993
    EXPECT_GT(r.scores.hate, 0.99);
    EXPECT_NEAR(r.max_confidence, r.scores.hate, 1e-9);
    // All other axes should be sigmoid(0) = 0.5
    EXPECT_NEAR(r.scores.sexual, 0.5, 1e-6);
    EXPECT_NEAR(r.scores.violence, 0.5, 1e-6);
}

TEST(LocalClassifierLayerTest, ClassifySigmoidStaysInRange) {
    // Extreme negative weights should produce probabilities near
    // 0 (not below). Extreme positive → near 1 (not above). This
    // catches any overflow/NaN in the sigmoid implementation.
    constexpr uint32_t num_cat = 8;
    constexpr uint32_t dim = 3;
    std::vector<float> w(num_cat * dim, 0.0f);
    w[0 * dim + 0] = -100.0f; // cat 0 very negative
    w[1 * dim + 0] = 100.0f;  // cat 1 very positive
    std::vector<float> b(num_cat, 0.0f);
    auto blob = build_blob(num_cat, dim, "test-model", w, b);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), "test-model"));

    auto r = layer.classify(make_embedding({1.0f, 0.0f, 0.0f}));
    ASSERT_TRUE(r.ran);
    // All axes in [0, 1]
    EXPECT_GE(r.scores.hate, 0.0);
    EXPECT_LE(r.scores.hate, 1.0);
    EXPECT_GE(r.scores.sexual, 0.0);
    EXPECT_LE(r.scores.sexual, 1.0);
    // Category 0 should be ~0, category 1 should be ~1
    EXPECT_LT(r.scores.hate, 1e-10);
    EXPECT_GT(r.scores.sexual, 1.0 - 1e-10);
}

TEST(LocalClassifierLayerTest, ClassifyZeroEmbeddingProducesHalf) {
    // Zero embedding + zero bias → logit=0 → sigmoid(0) = 0.5 for
    // every axis. max_confidence should be 0.5.
    constexpr uint32_t num_cat = 8;
    constexpr uint32_t dim = 3;
    std::vector<float> w(num_cat * dim, 0.0f);
    std::vector<float> b(num_cat, 0.0f);
    auto blob = build_blob(num_cat, dim, "test-model", w, b);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), "test-model"));

    auto r = layer.classify(make_embedding({0.0f, 0.0f, 0.0f}));
    ASSERT_TRUE(r.ran);
    EXPECT_NEAR(r.max_confidence, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.hate, 0.5, 1e-9);
}

TEST(LocalClassifierLayerTest, ClassifyRejectsDimMismatch) {
    // Weights are for dim=3, but we pass a dim=4 embedding.
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    auto blob = zero_blob();
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), "test-model"));

    auto r = layer.classify(make_embedding({0.1f, 0.2f, 0.3f, 0.4f}));
    EXPECT_FALSE(r.ran);
}

TEST(LocalClassifierLayerTest, ClassifyRejectsNotOkEmbedding) {
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    auto blob = zero_blob();
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), "test-model"));

    EmbeddingResult bad;
    bad.ok = false;
    bad.vector = {0.1f, 0.2f, 0.3f};
    auto r = layer.classify(bad);
    EXPECT_FALSE(r.ran);
}
