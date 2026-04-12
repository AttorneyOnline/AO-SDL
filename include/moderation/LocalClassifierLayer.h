/**
 * @file LocalClassifierLayer.h
 * @brief Layer 2: per-axis MLP classifier on embedding vectors.
 *
 * The local classifier is the PRIMARY moderation classifier in kagami.
 * For each message, it takes the bge-small embedding and runs per-axis
 * MLP inference to produce moderation scores. These scores feed directly
 * into the heat ladder via per-axis weights and floors configured in
 * HeatConfig.
 *
 * === Architecture (v2 format) ===
 *
 * Per-axis 2-layer MLP with residual skip connection:
 *
 *   hidden = ReLU(W1[axis] · x + b1[axis])
 *   logit  = W2[axis] · hidden + b2[axis] + W_skip[axis] · x
 *   prob   = sigmoid(platt_a[axis] * logit + platt_b[axis])
 *
 * The residual (W_skip · x) retains a direct linear path from input
 * to output — a safety net that prevents the MLP from overfitting
 * away from a working linear baseline on small training sets.
 * Platt calibration (optional, per-axis) maps raw logits to
 * well-calibrated probabilities.
 *
 * === Weight file format ===
 *
 * The loader supports both v1 (linear) and v2 (MLP) formats:
 *
 * v1 (legacy, ~12 KB):
 *   magic "KGCLF\x01\x00\x00" | version=1 | num_cat | dim |
 *   name_len | name | W[num_cat×dim] | b[num_cat]
 *   Inference: sigmoid(W[i]·x + b[i])
 *
 * v2 (MLP, ~800 KB):
 *   magic "KGCLF\x02\x00\x00" | version=2 | num_cat | dim |
 *   hidden_dim | name_len | name |
 *   W1[num_cat×dim×hidden] | b1[num_cat×hidden] |
 *   W2[num_cat×hidden] | b2[num_cat] |
 *   W_skip[num_cat×dim] |
 *   calibration_type(1 byte) |
 *   platt_a[num_cat] | platt_b[num_cat] (if calibration_type==1)
 *
 * === Embedding-model compatibility (critical) ===
 *
 * The classifier weights are ONLY meaningful in the latent space of
 * the specific embedding model they were trained against. The weights
 * file stores the HF model identifier; on mismatch, the layer
 * disables itself with a WARNING.
 */
#pragma once

#include "moderation/ContentModerationConfig.h"
#include "moderation/EmbeddingBackend.h"
#include "moderation/ModerationTypes.h"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace moderation {

struct LocalClassifierResult {
    bool ran = false;            ///< True iff the layer was active and classified.
    ModerationAxisScores scores; ///< Per-axis sigmoid outputs in [0, 1].
    double max_confidence = 0.0; ///< Maximum score across all axes.
    int max_category_index = -1; ///< Index of the strongest axis, -1 if none.
};

class LocalClassifierLayer {
  public:
    LocalClassifierLayer() = default;

    /// Apply scalar configuration (thresholds, enabled flag). Must be
    /// called before load_weights() so the thresholds are ready by
    /// the time the first classify() runs.
    void configure(const LocalClassifierConfig& cfg);

    /// Parse a weights blob into the in-memory matrix representation.
    ///
    /// The blob format is:
    ///   offset  size    field
    ///   ------  ----    -----
    ///   0       8       magic "KGCLF\x01\x00\x00"
    ///   8       4       format version (uint32 LE): currently 1
    ///   12      4       num_categories (uint32 LE)
    ///   16      4       embedding_dim (uint32 LE)
    ///   20      4       model_name_len (uint32 LE)
    ///   24      N       model_name (utf-8, not null-terminated)
    ///   24+N    M       weights (float32 LE, row-major num_cat x dim)
    ///   24+N+M  P       biases (float32 LE, num_categories)
    ///
    /// @p runtime_model_name is the embedding model id that kagami
    /// will actually use for message embeddings. If this mismatches
    /// the id stored in the weights file, the layer disables itself
    /// and logs a WARNING; the graceful-degrade path keeps the rest
    /// of the moderation stack running.
    ///
    /// Returns true on successful load (layer armed), false on any
    /// validation failure (layer remains inactive).
    bool load_weights(const uint8_t* blob, size_t blob_size, const std::string& runtime_model_name);

    /// Is the layer actually ready to classify? True only if
    /// configure() was called with enabled=true AND load_weights()
    /// succeeded with a model-name match.
    bool is_active() const;

    /// Number of categories in the loaded model. 0 when inactive.
    int num_categories() const;

    /// Embedding dimensionality the loaded weights expect. 0 when
    /// inactive. Used by the caller to sanity-check against the
    /// embedding backend's dimension().
    int embedding_dim() const;

    /// The HF model identifier stored in the loaded weights file.
    /// Empty when inactive. Exposed primarily so the caller can log
    /// it alongside mismatch warnings.
    const std::string& model_name() const;

    /// Hot path: classify a pre-computed embedding vector. Expects
    /// @p embedding to have is_ready=true, ok=true, and
    /// vector.size() == embedding_dim(). Returns a zeroed result
    /// with ran=false if the layer is inactive or the vector is
    /// malformed.
    LocalClassifierResult classify(const EmbeddingResult& embedding) const;

  private:
    mutable std::mutex mu_;
    LocalClassifierConfig cfg_{};

    int format_version_ = 0;
    int num_categories_ = 0;
    int embedding_dim_ = 0;
    int hidden_dim_ = 0;
    std::string model_name_;
    bool loaded_ = false;

    // === v1 (linear) storage ===
    // Flat row-major: [num_categories_ x embedding_dim_]
    std::vector<float> weights_;   // W for v1
    std::vector<float> biases_;    // b for v1

    // === v2 (MLP) storage ===
    // Per-axis independent MLPs: each axis has its own W1/b1/W2/b2/W_skip.
    // Stored contiguously per-layer across axes for cache-friendly access.
    std::vector<float> w1_;        // [num_cat × embedding_dim × hidden_dim]
    std::vector<float> b1_;        // [num_cat × hidden_dim]
    std::vector<float> w2_;        // [num_cat × hidden_dim]
    std::vector<float> b2_;        // [num_cat]
    std::vector<float> w_skip_;    // [num_cat × embedding_dim] (residual)
    int calibration_type_ = 0;     // 0=raw sigmoid, 1=platt
    std::vector<float> platt_a_;   // [num_cat] (platt scaling slope)
    std::vector<float> platt_b_;   // [num_cat] (platt scaling intercept)

    // Internal dispatch
    LocalClassifierResult classify_v1(const EmbeddingResult& embedding) const;
    LocalClassifierResult classify_v2(const EmbeddingResult& embedding) const;

    bool load_weights_v1(const uint8_t* blob, size_t blob_size, uint32_t num_cat,
                         uint32_t dim, uint32_t name_len, const std::string& runtime_model_name);
    bool load_weights_v2(const uint8_t* blob, size_t blob_size, uint32_t num_cat,
                         uint32_t dim, uint32_t name_len, const std::string& runtime_model_name);
};

} // namespace moderation
