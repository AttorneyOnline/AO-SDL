#include "moderation/LocalClassifierLayer.h"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <string>
#include <thread>
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

// ====================================================================
// V2 (MLP) test helpers and tests
// ====================================================================

namespace {

/// Helper sigmoid matching the production implementation for hand-computing
/// expected values in tests.
double test_sigmoid(double x) {
    if (x >= 0.0) {
        const double z = std::exp(-x);
        return 1.0 / (1.0 + z);
    }
    const double z = std::exp(x);
    return z / (1.0 + z);
}

/// Build a valid v2 (MLP) weights blob in memory.
///
/// Layout: magic "KGCLF\x02\x00\x00" | version=2 | num_cat | dim |
///         hidden_dim | name_len | name |
///         W1[num_cat*dim*hidden] | b1[num_cat*hidden] |
///         W2[num_cat*hidden] | b2[num_cat] |
///         W_skip[num_cat*dim] |
///         calibration_type(1 byte) |
///         platt_a[num_cat] (if calibration_type==1) |
///         platt_b[num_cat] (if calibration_type==1)
struct V2BlobParams {
    uint32_t num_cat = 5;
    uint32_t dim = 3;
    uint32_t hidden = 2;
    std::string model_name = "test-model";
    std::vector<float> w1;     // [num_cat * dim * hidden]
    std::vector<float> b1;     // [num_cat * hidden]
    std::vector<float> w2;     // [num_cat * hidden]
    std::vector<float> b2;     // [num_cat]
    std::vector<float> w_skip; // [num_cat * dim]
    int calibration_type = 0;
    std::vector<float> platt_a; // [num_cat] if calibration_type==1
    std::vector<float> platt_b; // [num_cat] if calibration_type==1
};

std::vector<uint8_t> build_v2_blob(const V2BlobParams& p) {
    std::vector<uint8_t> blob;

    auto push_u8 = [&](uint8_t v) { blob.push_back(v); };
    auto push_u32 = [&](uint32_t v) {
        blob.push_back(static_cast<uint8_t>(v & 0xff));
        blob.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
        blob.push_back(static_cast<uint8_t>((v >> 16) & 0xff));
        blob.push_back(static_cast<uint8_t>((v >> 24) & 0xff));
    };
    auto push_floats = [&](const std::vector<float>& v) {
        const auto* bytes = reinterpret_cast<const uint8_t*>(v.data());
        blob.insert(blob.end(), bytes, bytes + v.size() * sizeof(float));
    };

    // Magic: "KGCLF" + 0x02 0x00 0x00
    const char magic[] = "KGCLF";
    blob.insert(blob.end(), magic, magic + 5);
    push_u8(0x02);
    push_u8(0x00);
    push_u8(0x00);

    // Header fields
    push_u32(2); // version
    push_u32(p.num_cat);
    push_u32(p.dim);
    push_u32(p.hidden);
    push_u32(static_cast<uint32_t>(p.model_name.size()));

    // Model name
    blob.insert(blob.end(), p.model_name.begin(), p.model_name.end());

    // Weight tensors
    push_floats(p.w1);
    push_floats(p.b1);
    push_floats(p.w2);
    push_floats(p.b2);
    push_floats(p.w_skip);

    // Calibration
    push_u8(static_cast<uint8_t>(p.calibration_type));
    if (p.calibration_type == 1) {
        push_floats(p.platt_a);
        push_floats(p.platt_b);
    }

    return blob;
}

/// Build a minimal all-zero v2 blob. num_cat defaults to 5 (post-toxicity-
/// removal axis count: hate, sexual, sexual_minors, violence, self_harm).
V2BlobParams zero_v2_params(const std::string& model_name = "test-model", uint32_t num_cat = 5, uint32_t dim = 3,
                            uint32_t hidden = 2) {
    V2BlobParams p;
    p.num_cat = num_cat;
    p.dim = dim;
    p.hidden = hidden;
    p.model_name = model_name;
    p.w1.assign(num_cat * dim * hidden, 0.0f);
    p.b1.assign(num_cat * hidden, 0.0f);
    p.w2.assign(num_cat * hidden, 0.0f);
    p.b2.assign(num_cat, 0.0f);
    p.w_skip.assign(num_cat * dim, 0.0f);
    p.calibration_type = 0;
    return p;
}

} // namespace

// ====================================================================
// V2 loading tests
// ====================================================================

TEST(LocalClassifierLayerV2Test, LoadsValidV2Blob) {
    auto params = zero_v2_params();
    auto blob = build_v2_blob(params);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), "test-model"));
    EXPECT_TRUE(layer.is_active());
    EXPECT_EQ(layer.num_categories(), 5);
    EXPECT_EQ(layer.embedding_dim(), 3);
    EXPECT_EQ(layer.model_name(), "test-model");
}

TEST(LocalClassifierLayerV2Test, RejectsWrongMagic) {
    auto params = zero_v2_params();
    auto blob = build_v2_blob(params);
    blob[0] = 'Z'; // corrupt first magic byte

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    EXPECT_FALSE(layer.load_weights(blob.data(), blob.size(), "test-model"));
    EXPECT_FALSE(layer.is_active());
}

TEST(LocalClassifierLayerV2Test, RejectsTruncatedBlob) {
    auto params = zero_v2_params();
    auto blob = build_v2_blob(params);
    // Chop off the last 8 bytes (part of w_skip or calibration)
    blob.resize(blob.size() - 8);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    EXPECT_FALSE(layer.load_weights(blob.data(), blob.size(), "test-model"));
    EXPECT_FALSE(layer.is_active());
}

TEST(LocalClassifierLayerV2Test, RejectsTruncatedHeader) {
    // A blob too short to even hold the v2 fixed header (28 bytes).
    std::vector<uint8_t> blob(20, 0);
    // Write enough of the magic+version so the parser enters the v2 branch.
    const char magic[] = "KGCLF";
    std::memcpy(blob.data(), magic, 5);
    blob[5] = 0x02;
    // version=2 at offset 8
    blob[8] = 0x02;
    // num_cat=1 at offset 12
    blob[12] = 0x01;
    // dim=1 at offset 16
    blob[16] = 0x01;
    // blob is only 20 bytes, less than kV2HeaderFixedSize (28)

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    EXPECT_FALSE(layer.load_weights(blob.data(), blob.size(), ""));
}

TEST(LocalClassifierLayerV2Test, RejectsModelNameMismatch) {
    auto params = zero_v2_params("model-alpha");
    auto blob = build_v2_blob(params);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    EXPECT_FALSE(layer.load_weights(blob.data(), blob.size(), "model-beta"));
    EXPECT_FALSE(layer.is_active());
}

TEST(LocalClassifierLayerV2Test, ColonSuffixStrippingWorks) {
    // Weights file stores "hf-user/my-model", runtime passes
    // "hf-user/my-model:q8_0.gguf". The colon-suffix stripping
    // logic should make this match.
    auto params = zero_v2_params("hf-user/my-model");
    auto blob = build_v2_blob(params);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    EXPECT_TRUE(layer.load_weights(blob.data(), blob.size(), "hf-user/my-model:q8_0.gguf"));
    EXPECT_TRUE(layer.is_active());
}

TEST(LocalClassifierLayerV2Test, RejectsHiddenDimZero) {
    auto params = zero_v2_params();
    params.hidden = 0;
    // Rebuild with hidden=0 in the header but keep arrays sized for hidden=2
    // so the blob is "valid" except for the header field.
    auto blob = build_v2_blob(params);
    // Patch hidden_dim to 0 at offset 20
    blob[20] = 0;
    blob[21] = 0;
    blob[22] = 0;
    blob[23] = 0;

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    EXPECT_FALSE(layer.load_weights(blob.data(), blob.size(), ""));
}

TEST(LocalClassifierLayerV2Test, RejectsHiddenDimTooLarge) {
    auto params = zero_v2_params();
    auto blob = build_v2_blob(params);
    // Patch hidden_dim to 2000 (> 1024 limit) at offset 20
    uint32_t bad_hidden = 2000;
    blob[20] = static_cast<uint8_t>(bad_hidden & 0xff);
    blob[21] = static_cast<uint8_t>((bad_hidden >> 8) & 0xff);
    blob[22] = static_cast<uint8_t>((bad_hidden >> 16) & 0xff);
    blob[23] = static_cast<uint8_t>((bad_hidden >> 24) & 0xff);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    EXPECT_FALSE(layer.load_weights(blob.data(), blob.size(), ""));
}

TEST(LocalClassifierLayerV2Test, RejectsZeroCategories) {
    auto params = zero_v2_params();
    auto blob = build_v2_blob(params);
    // Patch num_categories to 0 at offset 12
    blob[12] = 0;
    blob[13] = 0;
    blob[14] = 0;
    blob[15] = 0;

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    EXPECT_FALSE(layer.load_weights(blob.data(), blob.size(), ""));
}

// ====================================================================
// V2 MLP inference tests
// ====================================================================

TEST(LocalClassifierLayerV2Test, ZeroWeightsProduceHalf) {
    // All weights/biases zero → logit=0 for every axis → sigmoid(0)=0.5.
    auto params = zero_v2_params();
    auto blob = build_v2_blob(params);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), ""));

    auto r = layer.classify(make_embedding({1.0f, 2.0f, 3.0f}));
    ASSERT_TRUE(r.ran);
    EXPECT_NEAR(r.scores.hate, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.sexual, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.sexual_minors, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.violence, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.self_harm, 0.5, 1e-9);
    EXPECT_NEAR(r.max_confidence, 0.5, 1e-9);
}

TEST(LocalClassifierLayerV2Test, HandComputedMLPForwardPass) {
    // 1 axis (hate), dim=2, hidden=2.
    // This lets us hand-compute every step.
    //
    // Weights:
    //   W1 = [[1, -1],   (W1[0*H+0]=1, W1[0*H+1]=-1,
    //          [0,  2]]    W1[1*H+0]=0, W1[1*H+1]=2)
    //   — stored as w1_[j*H + k] for axis 0
    //   b1 = [0.5, -0.5]
    //   W2 = [1, 0.5]
    //   b2 = [-1]
    //   W_skip = [0.1, 0.2]
    //
    // Input x = [1.0, 0.5]
    //
    // Layer 1:
    //   hidden[0] = ReLU(W1[0,0]*x[0] + W1[1,0]*x[1] + b1[0])
    //             = ReLU(1*1 + 0*0.5 + 0.5) = ReLU(1.5) = 1.5
    //   hidden[1] = ReLU(W1[0,1]*x[0] + W1[1,1]*x[1] + b1[1])
    //             = ReLU(-1*1 + 2*0.5 + (-0.5)) = ReLU(-0.5) = 0.0
    //
    // Layer 2:
    //   logit = W2[0]*hidden[0] + W2[1]*hidden[1] + b2
    //         + W_skip[0]*x[0] + W_skip[1]*x[1]
    //         = 1*1.5 + 0.5*0 + (-1) + 0.1*1 + 0.2*0.5
    //         = 1.5 + 0 - 1 + 0.1 + 0.1 = 0.7
    //
    // prob = sigmoid(0.7)

    const uint32_t nc = 1, dim = 2, hid = 2;
    V2BlobParams p;
    p.num_cat = nc;
    p.dim = dim;
    p.hidden = hid;
    p.model_name = "test-model";
    // W1: row-major [dim × hidden] = [[1, -1], [0, 2]]
    p.w1 = {1.0f, -1.0f, // j=0: W1[0*H+0]=1, W1[0*H+1]=-1
            0.0f, 2.0f}; // j=1: W1[1*H+0]=0, W1[1*H+1]=2
    p.b1 = {0.5f, -0.5f};
    p.w2 = {1.0f, 0.5f};
    p.b2 = {-1.0f};
    p.w_skip = {0.1f, 0.2f};
    p.calibration_type = 0;
    auto blob = build_v2_blob(p);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), ""));

    auto r = layer.classify(make_embedding({1.0f, 0.5f}));
    ASSERT_TRUE(r.ran);

    const double expected_logit = 0.7;
    const double expected_prob = test_sigmoid(expected_logit);
    EXPECT_NEAR(r.scores.hate, expected_prob, 1e-9);
    EXPECT_NEAR(r.max_confidence, expected_prob, 1e-9);
    EXPECT_EQ(r.max_category_index, 0);
}

TEST(LocalClassifierLayerV2Test, ResidualSkipAlone) {
    // Test the residual skip connection in isolation.
    // Set W1=0, W2=0, b1=0 so the MLP path contributes nothing.
    // Only W_skip and b2 remain:
    //   logit = W_skip · x + b2
    //   prob  = sigmoid(logit)
    //
    // 1 axis, dim=2, hidden=2.
    // W_skip = [3.0, -1.0], b2 = [0.5]
    // x = [1.0, 2.0]
    // logit = 3*1 + (-1)*2 + 0.5 = 1.5
    // prob = sigmoid(1.5)

    const uint32_t nc = 1, dim = 2, hid = 2;
    V2BlobParams p;
    p.num_cat = nc;
    p.dim = dim;
    p.hidden = hid;
    p.model_name = "test-model";
    p.w1.assign(nc * dim * hid, 0.0f);
    p.b1.assign(nc * hid, 0.0f);
    p.w2.assign(nc * hid, 0.0f);
    p.b2 = {0.5f};
    p.w_skip = {3.0f, -1.0f};
    p.calibration_type = 0;
    auto blob = build_v2_blob(p);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), ""));

    auto r = layer.classify(make_embedding({1.0f, 2.0f}));
    ASSERT_TRUE(r.ran);

    const double expected = test_sigmoid(3.0 * 1.0 + (-1.0) * 2.0 + 0.5);
    EXPECT_NEAR(r.scores.hate, expected, 1e-9);
}

TEST(LocalClassifierLayerV2Test, ReLUZerosNegativeHiddenValues) {
    // Verify the ReLU activation: when all hidden pre-activations are
    // negative, the hidden layer output is all zeros, so the MLP path
    // contributes nothing and only the skip/bias remain.
    //
    // 1 axis, dim=1, hidden=2.
    // W1 = [-10, -10], b1 = [-5, -5]  → both hidden nodes deeply negative
    // W2 = [100, 100]  (large, but multiplied by 0 after ReLU)
    // b2 = [0], W_skip = [0]
    //
    // x = [1.0]
    // hidden = ReLU([-10*1 - 5, -10*1 - 5]) = ReLU([-15, -15]) = [0, 0]
    // logit = 100*0 + 100*0 + 0 + 0*1 = 0
    // prob = sigmoid(0) = 0.5

    const uint32_t nc = 1, dim = 1, hid = 2;
    V2BlobParams p;
    p.num_cat = nc;
    p.dim = dim;
    p.hidden = hid;
    p.model_name = "test-model";
    p.w1 = {-10.0f, -10.0f}; // j=0: W1[0*H+0]=-10, W1[0*H+1]=-10
    p.b1 = {-5.0f, -5.0f};
    p.w2 = {100.0f, 100.0f};
    p.b2 = {0.0f};
    p.w_skip = {0.0f};
    p.calibration_type = 0;
    auto blob = build_v2_blob(p);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), ""));

    auto r = layer.classify(make_embedding({1.0f}));
    ASSERT_TRUE(r.ran);
    EXPECT_NEAR(r.scores.hate, 0.5, 1e-9);
}

TEST(LocalClassifierLayerV2Test, AllScoresInZeroOneRange) {
    // Large positive and negative weights should still produce scores
    // clamped to [0, 1] by the sigmoid. Catches overflow/NaN.
    const uint32_t nc = 5, dim = 2, hid = 2;
    V2BlobParams p;
    p.num_cat = nc;
    p.dim = dim;
    p.hidden = hid;
    p.model_name = "test-model";
    p.w1.assign(nc * dim * hid, 0.0f);
    p.b1.assign(nc * hid, 0.0f);
    p.w2.assign(nc * hid, 0.0f);
    p.b2.assign(nc, 0.0f);
    p.w_skip.assign(nc * dim, 0.0f);
    // Axis 0: extreme positive skip weight
    p.w_skip[0] = 500.0f;
    // Axis 1: extreme negative skip weight
    p.w_skip[1 * dim + 0] = -500.0f;
    p.calibration_type = 0;
    auto blob = build_v2_blob(p);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), ""));

    auto r = layer.classify(make_embedding({1.0f, 0.0f}));
    ASSERT_TRUE(r.ran);
    EXPECT_GE(r.scores.hate, 0.0);
    EXPECT_LE(r.scores.hate, 1.0);
    EXPECT_GE(r.scores.sexual, 0.0);
    EXPECT_LE(r.scores.sexual, 1.0);
    EXPECT_GE(r.scores.sexual_minors, 0.0);
    EXPECT_LE(r.scores.sexual_minors, 1.0);
    EXPECT_GE(r.scores.violence, 0.0);
    EXPECT_LE(r.scores.violence, 1.0);
    EXPECT_GE(r.scores.self_harm, 0.0);
    EXPECT_LE(r.scores.self_harm, 1.0);
    // Axis 0 should be ~1, axis 1 should be ~0
    EXPECT_GT(r.scores.hate, 1.0 - 1e-10);
    EXPECT_LT(r.scores.sexual, 1e-10);
}

TEST(LocalClassifierLayerV2Test, MultiAxisIndependentInference) {
    // Two axes (num_cat=2), each with known different weights.
    // Verify they produce different independent scores.
    //
    // dim=1, hidden=1 for simplicity.
    // Axis 0: W1=[2], b1=[0], W2=[1], b2=[0], W_skip=[0]
    //   hidden = ReLU(2*x + 0), logit = 1*hidden + 0 + 0 = 2*x (if x>0)
    //
    // Axis 1: W1=[0], b1=[0], W2=[0], b2=[0], W_skip=[5]
    //   hidden = ReLU(0), logit = 0 + 0 + 5*x = 5*x
    //
    // x = [1.0]
    // Axis 0: logit = 2.0, prob = sigmoid(2.0)
    // Axis 1: logit = 5.0, prob = sigmoid(5.0)

    const uint32_t nc = 2, dim = 1, hid = 1;
    V2BlobParams p;
    p.num_cat = nc;
    p.dim = dim;
    p.hidden = hid;
    p.model_name = "test-model";
    // W1: axis 0 = [2.0], axis 1 = [0.0]
    p.w1 = {2.0f, 0.0f};
    p.b1 = {0.0f, 0.0f};
    p.w2 = {1.0f, 0.0f};
    p.b2 = {0.0f, 0.0f};
    p.w_skip = {0.0f, 5.0f};
    p.calibration_type = 0;
    auto blob = build_v2_blob(p);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), ""));

    auto r = layer.classify(make_embedding({1.0f}));
    ASSERT_TRUE(r.ran);
    EXPECT_NEAR(r.scores.hate, test_sigmoid(2.0), 1e-9);
    EXPECT_NEAR(r.scores.sexual, test_sigmoid(5.0), 1e-9);
    // Axis 1 (sexual) should be the max
    EXPECT_EQ(r.max_category_index, 1);
    EXPECT_NEAR(r.max_confidence, test_sigmoid(5.0), 1e-9);
}

// ====================================================================
// V2 Platt calibration tests
// ====================================================================

TEST(LocalClassifierLayerV2Test, PlattCalibrationApplied) {
    // With calibration_type=1, output should be sigmoid(a*logit + b)
    // instead of sigmoid(logit).
    //
    // 1 axis, dim=1, hidden=1.
    // All MLP weights zero except W_skip=[1.0], b2=[0].
    // So raw logit = W_skip · x = 1.0 * x[0].
    //
    // Platt: a=2.0, b=-1.0
    // x=[3.0] → logit=3.0 → calibrated = sigmoid(2*3 + (-1)) = sigmoid(5)

    const uint32_t nc = 1, dim = 1, hid = 1;
    V2BlobParams p;
    p.num_cat = nc;
    p.dim = dim;
    p.hidden = hid;
    p.model_name = "test-model";
    p.w1 = {0.0f};
    p.b1 = {0.0f};
    p.w2 = {0.0f};
    p.b2 = {0.0f};
    p.w_skip = {1.0f};
    p.calibration_type = 1;
    p.platt_a = {2.0f};
    p.platt_b = {-1.0f};
    auto blob = build_v2_blob(p);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), ""));

    auto r = layer.classify(make_embedding({3.0f}));
    ASSERT_TRUE(r.ran);

    // logit = 3.0, calibrated = sigmoid(2*3 - 1) = sigmoid(5)
    const double expected = test_sigmoid(2.0 * 3.0 + (-1.0));
    EXPECT_NEAR(r.scores.hate, expected, 1e-9);
}

TEST(LocalClassifierLayerV2Test, NoCalibrationGivesRawSigmoid) {
    // calibration_type=0 → raw sigmoid(logit), no Platt transform.
    // Same setup as above but without calibration.

    const uint32_t nc = 1, dim = 1, hid = 1;
    V2BlobParams p;
    p.num_cat = nc;
    p.dim = dim;
    p.hidden = hid;
    p.model_name = "test-model";
    p.w1 = {0.0f};
    p.b1 = {0.0f};
    p.w2 = {0.0f};
    p.b2 = {0.0f};
    p.w_skip = {1.0f};
    p.calibration_type = 0;
    auto blob = build_v2_blob(p);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), ""));

    auto r = layer.classify(make_embedding({3.0f}));
    ASSERT_TRUE(r.ran);
    EXPECT_NEAR(r.scores.hate, test_sigmoid(3.0), 1e-9);
}

TEST(LocalClassifierLayerV2Test, PlattCalibrationPerAxis) {
    // Verify Platt scaling is applied independently per axis.
    // 2 axes, each with different platt_a/platt_b.
    //
    // Both axes: raw logit = 2.0 (via W_skip=[2], x=[1])
    // Axis 0: platt_a=1.0, platt_b=0.0 → sigmoid(1*2 + 0) = sigmoid(2)
    // Axis 1: platt_a=-0.5, platt_b=3.0 → sigmoid(-0.5*2 + 3) = sigmoid(2)
    // (They happen to match here — but the path is different.)

    const uint32_t nc = 2, dim = 1, hid = 1;
    V2BlobParams p;
    p.num_cat = nc;
    p.dim = dim;
    p.hidden = hid;
    p.model_name = "test-model";
    p.w1 = {0.0f, 0.0f};
    p.b1 = {0.0f, 0.0f};
    p.w2 = {0.0f, 0.0f};
    p.b2 = {0.0f, 0.0f};
    p.w_skip = {2.0f, 2.0f};
    p.calibration_type = 1;
    p.platt_a = {1.0f, -0.5f};
    p.platt_b = {0.0f, 3.0f};
    auto blob = build_v2_blob(p);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), ""));

    auto r = layer.classify(make_embedding({1.0f}));
    ASSERT_TRUE(r.ran);

    // Both logits = 2.0
    EXPECT_NEAR(r.scores.hate, test_sigmoid(1.0 * 2.0 + 0.0), 1e-9);
    EXPECT_NEAR(r.scores.sexual, test_sigmoid(-0.5 * 2.0 + 3.0), 1e-9);
}

TEST(LocalClassifierLayerV2Test, TruncatedPlattDataRejected) {
    // calibration_type=1 but blob is too short for platt_a/platt_b arrays.
    auto params = zero_v2_params();
    params.calibration_type = 1;
    params.platt_a.assign(params.num_cat, 1.0f);
    params.platt_b.assign(params.num_cat, 0.0f);
    auto blob = build_v2_blob(params);

    // Chop off the platt_b array (last num_cat * 4 bytes)
    blob.resize(blob.size() - params.num_cat * sizeof(float));

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    EXPECT_FALSE(layer.load_weights(blob.data(), blob.size(), ""));
    EXPECT_FALSE(layer.is_active());
}

// ====================================================================
// Axis index mapping tests (post-toxicity-removal)
// ====================================================================

TEST(LocalClassifierLayerV2Test, AxisIndex0MapsToHate) {
    // Only axis 0 has nonzero W_skip; verify scores.hate is populated.
    const uint32_t nc = 5, dim = 1, hid = 1;
    V2BlobParams p = zero_v2_params("test-model", nc, dim, hid);
    p.w_skip[0 * dim] = 5.0f; // axis 0

    auto blob = build_v2_blob(p);
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), ""));

    auto r = layer.classify(make_embedding({1.0f}));
    ASSERT_TRUE(r.ran);
    EXPECT_NEAR(r.scores.hate, test_sigmoid(5.0), 1e-9);
    EXPECT_NEAR(r.scores.sexual, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.sexual_minors, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.violence, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.self_harm, 0.5, 1e-9);
    EXPECT_EQ(r.max_category_index, 0);
}

TEST(LocalClassifierLayerV2Test, AxisIndex1MapsToSexual) {
    const uint32_t nc = 5, dim = 1, hid = 1;
    V2BlobParams p = zero_v2_params("test-model", nc, dim, hid);
    p.w_skip[1 * dim] = 5.0f; // axis 1

    auto blob = build_v2_blob(p);
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), ""));

    auto r = layer.classify(make_embedding({1.0f}));
    ASSERT_TRUE(r.ran);
    EXPECT_NEAR(r.scores.hate, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.sexual, test_sigmoid(5.0), 1e-9);
    EXPECT_NEAR(r.scores.sexual_minors, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.violence, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.self_harm, 0.5, 1e-9);
    EXPECT_EQ(r.max_category_index, 1);
}

TEST(LocalClassifierLayerV2Test, AxisIndex2MapsToSexualMinors) {
    const uint32_t nc = 5, dim = 1, hid = 1;
    V2BlobParams p = zero_v2_params("test-model", nc, dim, hid);
    p.w_skip[2 * dim] = 5.0f; // axis 2

    auto blob = build_v2_blob(p);
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), ""));

    auto r = layer.classify(make_embedding({1.0f}));
    ASSERT_TRUE(r.ran);
    EXPECT_NEAR(r.scores.hate, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.sexual, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.sexual_minors, test_sigmoid(5.0), 1e-9);
    EXPECT_NEAR(r.scores.violence, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.self_harm, 0.5, 1e-9);
    EXPECT_EQ(r.max_category_index, 2);
}

TEST(LocalClassifierLayerV2Test, AxisIndex3MapsToViolence) {
    const uint32_t nc = 5, dim = 1, hid = 1;
    V2BlobParams p = zero_v2_params("test-model", nc, dim, hid);
    p.w_skip[3 * dim] = 5.0f; // axis 3

    auto blob = build_v2_blob(p);
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), ""));

    auto r = layer.classify(make_embedding({1.0f}));
    ASSERT_TRUE(r.ran);
    EXPECT_NEAR(r.scores.hate, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.sexual, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.sexual_minors, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.violence, test_sigmoid(5.0), 1e-9);
    EXPECT_NEAR(r.scores.self_harm, 0.5, 1e-9);
    EXPECT_EQ(r.max_category_index, 3);
}

TEST(LocalClassifierLayerV2Test, AxisIndex4MapsToSelfHarm) {
    const uint32_t nc = 5, dim = 1, hid = 1;
    V2BlobParams p = zero_v2_params("test-model", nc, dim, hid);
    p.w_skip[4 * dim] = 5.0f; // axis 4

    auto blob = build_v2_blob(p);
    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), ""));

    auto r = layer.classify(make_embedding({1.0f}));
    ASSERT_TRUE(r.ran);
    EXPECT_NEAR(r.scores.hate, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.sexual, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.sexual_minors, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.violence, 0.5, 1e-9);
    EXPECT_NEAR(r.scores.self_harm, test_sigmoid(5.0), 1e-9);
    EXPECT_EQ(r.max_category_index, 4);
}

// ====================================================================
// V2 dimension mismatch test
// ====================================================================

TEST(LocalClassifierLayerV2Test, ClassifyRejectsDimMismatch) {
    auto params = zero_v2_params("test-model", 5, 3, 2);
    auto blob = build_v2_blob(params);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), ""));

    // Model expects dim=3, but we pass dim=4.
    auto r = layer.classify(make_embedding({1.0f, 2.0f, 3.0f, 4.0f}));
    EXPECT_FALSE(r.ran);
}

// ====================================================================
// V2 replaces V1 on reload
// ====================================================================

TEST(LocalClassifierLayerV2Test, V2ReloadClearsV1State) {
    // Load v1, then reload with v2. The layer should use v2 inference.
    constexpr uint32_t v1_nc = 8, v1_dim = 3;
    std::vector<float> v1_w(v1_nc * v1_dim, 0.0f);
    v1_w[0] = 10.0f; // axis 0, very high on v1
    std::vector<float> v1_b(v1_nc, 0.0f);
    auto v1_blob = build_blob(v1_nc, v1_dim, "test-model", v1_w, v1_b);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(v1_blob.data(), v1_blob.size(), ""));

    // Now load v2 with all-zero weights.
    auto v2_params = zero_v2_params("test-model", 5, 3, 2);
    auto v2_blob = build_v2_blob(v2_params);
    ASSERT_TRUE(layer.load_weights(v2_blob.data(), v2_blob.size(), ""));

    // All scores should be 0.5 (v2 zero weights), not high (v1 leftover).
    auto r = layer.classify(make_embedding({1.0f, 0.0f, 0.0f}));
    ASSERT_TRUE(r.ran);
    EXPECT_NEAR(r.scores.hate, 0.5, 1e-9);
}

// ====================================================================
// Thread safety
// ====================================================================

TEST(LocalClassifierLayerV2Test, ConcurrentConfigureAndClassify) {
    // Hammer configure() and classify() from different threads.
    // We're just checking this doesn't crash or deadlock, not
    // verifying specific interleaving results.

    auto params = zero_v2_params("test-model", 5, 3, 2);
    auto blob = build_v2_blob(params);

    LocalClassifierLayer layer;
    layer.configure(enabled_cfg());
    ASSERT_TRUE(layer.load_weights(blob.data(), blob.size(), ""));

    constexpr int kIterations = 500;
    auto embedding = make_embedding({1.0f, 2.0f, 3.0f});

    std::thread classifier([&] {
        for (int i = 0; i < kIterations; ++i) {
            auto r = layer.classify(embedding);
            // Result is either valid or not, depending on race with configure.
            // Just make sure we don't crash.
            (void)r;
        }
    });

    std::thread configurer([&] {
        LocalClassifierConfig on_cfg;
        on_cfg.enabled = true;
        on_cfg.confidence_high_skip = 0.9;
        on_cfg.confidence_low_clean = 0.2;
        LocalClassifierConfig off_cfg;
        off_cfg.enabled = false;
        for (int i = 0; i < kIterations; ++i) {
            layer.configure(i % 2 == 0 ? on_cfg : off_cfg);
        }
    });

    classifier.join();
    configurer.join();
    // If we got here without crashing or hanging, the test passes.
}
