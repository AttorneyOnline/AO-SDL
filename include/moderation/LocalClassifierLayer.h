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
 * === Weight file format (v2) ===
 *
 *   magic "KGCLF\x02\x00\x00" | version=2 | num_cat | dim |
 *   hidden_dim | name_len | name |
 *   W1[num_cat×dim×hidden] | b1[num_cat×hidden] |
 *   W2[num_cat×hidden] | b2[num_cat] |
 *   W_skip[num_cat×dim] |
 *   calibration_type(1 byte) |
 *   platt_a[num_cat] | platt_b[num_cat] (if calibration_type==1)
 *
 * Note: W1 is stored on disk as [dim×hidden] but transposed to
 * [hidden×dim] in memory for cache-friendly inner-loop access.
 *
 * === Embedding-model compatibility (critical) ===
 *
 * The classifier weights are ONLY meaningful in the latent space of
 * the specific embedding model they were trained against. The weights
 * file stores the HF model identifier; on mismatch, the layer
 * disables itself with a WARNING.
 *
 * === Concurrency ===
 *
 * Weights are loaded into an immutable struct behind shared_ptr.
 * classify() snapshots the pointer (atomic load) and reads without
 * any lock. load_weights() builds a new struct and atomically swaps
 * the pointer. This is the standard "rarely updated, frequently read"
 * pattern — no serialization on the hot path.
 */
#pragma once

#include "moderation/ContentModerationConfig.h"
#include "moderation/EmbeddingBackend.h"
#include "moderation/ModerationTypes.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
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

    void configure(const LocalClassifierConfig& cfg);

    bool load_weights(const uint8_t* blob, size_t blob_size, const std::string& runtime_model_name);

    bool is_active() const;
    int num_categories() const;
    int embedding_dim() const;
    const std::string& model_name() const;

    /// Hot path: classify a pre-computed embedding vector. Lock-free —
    /// snapshots the current weights via atomic shared_ptr load.
    LocalClassifierResult classify(const EmbeddingResult& embedding) const;

  private:
    /// Immutable weight data. Built once by load_weights(), then shared
    /// across all classify() calls via shared_ptr. Never mutated after
    /// construction.
    struct Weights {
        int num_categories = 0;
        int embedding_dim = 0;
        int hidden_dim = 0;
        std::string model_name;

        // W1 is transposed from on-disk [dim×hidden] to in-memory
        // [hidden×dim] for cache-friendly access in the inner loop.
        std::vector<float> w1;      // [num_cat × hidden_dim × embedding_dim]
        std::vector<float> b1;      // [num_cat × hidden_dim]
        std::vector<float> w2;      // [num_cat × hidden_dim]
        std::vector<float> b2;      // [num_cat]
        std::vector<float> w_skip;  // [num_cat × embedding_dim] (residual)
        int calibration_type = 0;   // 0=raw sigmoid, 1=platt
        std::vector<float> platt_a; // [num_cat]
        std::vector<float> platt_b; // [num_cat]
    };

    mutable std::shared_mutex mu_;
    LocalClassifierConfig cfg_{};
    std::shared_ptr<const Weights> weights_{};

    static LocalClassifierResult classify_impl(const Weights& w, const EmbeddingResult& embedding);
};

} // namespace moderation
