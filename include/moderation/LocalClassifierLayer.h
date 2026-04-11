/**
 * @file LocalClassifierLayer.h
 * @brief Layer 2 shortcut: thin linear classifier on embedding vectors.
 *
 * This layer sits between the safe-hint shortcut and the remote
 * classifier in the Layer 2 decision tree. For each message it takes
 * the embedding vector that SafeHintLayer already computes, runs a
 * single 384×8 matrix-vector multiply + sigmoid, and produces per-
 * axis probabilities. Two exits short-circuit the remote call:
 *
 *   - HIGH CONFIDENCE BAD: any axis above confidence_high_skip →
 *     Layer 2 is already sure this is bad; inject the score into
 *     the verdict and skip the OpenAI call. Feeds `local_classifier_bad`
 *     into the l2_skipped_counter.
 *
 *   - HIGH CONFIDENCE CLEAN: maximum axis below confidence_low_clean
 *     → Layer 2 is already sure this is fine; skip the OpenAI call.
 *     Feeds `local_classifier_clean` into the l2_skipped_counter.
 *
 * Messages in the middle band (some axis between low and high) are
 * genuinely uncertain and escalate to the remote classifier for a
 * tiebreaker. The local classifier does not REPLACE the remote —
 * it HANDLES the obvious cases so the remote becomes a last-resort
 * verifier, not a first-line decoder.
 *
 * ===============================================================
 * Embedding-model compatibility (critical)
 * ===============================================================
 *
 * The classifier weights are ONLY meaningful in the latent space of
 * the specific embedding model they were trained against. A runtime
 * mismatch (e.g. operator overrides embeddings.hf_model_id to a
 * different 384-dim model) produces silently-wrong classifications —
 * the vector shape is fine but the basis vectors of the embedding
 * space are different, so the trained decision boundary is pure
 * noise relative to the new model.
 *
 * The weights file format (see scripts/train_classifier.py for the
 * full layout) stores the full HuggingFace model identifier in the
 * header. At load time, load_weights() compares that identifier
 * against the passed-in `runtime_model_name`. On mismatch, the
 * layer DISABLES itself — is_active() returns false regardless of
 * config, a WARNING is logged with both names, and ContentModerator
 * behaves as if the layer were off. The graceful-degrade path keeps
 * moderation running with the remaining layers; the operator gets
 * a clear pointer to scripts/train_classifier.py for remediation.
 *
 * If you are reading this because of an operator complaint about
 * the local_classifier being inactive: the fix is to run
 * `python3 scripts/train_classifier.py --embedding-model <new-id>`
 * and rebuild kagami. Weights are a 12 KB binary file bundled into
 * the image via cmake/EmbedAssets.cmake.
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

    // Flat row-major weights: [num_categories_ x embedding_dim_].
    // Kept as std::vector<float> for cache-friendly traversal in the
    // GEMV loop. The biases trail the weights in a separate vector.
    std::vector<float> weights_;
    std::vector<float> biases_;
    int num_categories_ = 0;
    int embedding_dim_ = 0;
    std::string model_name_;
    bool loaded_ = false;
};

} // namespace moderation
