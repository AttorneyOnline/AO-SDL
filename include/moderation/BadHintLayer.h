/**
 * @file BadHintLayer.h
 * @brief Layer 2 shortcut: detect messages close to known-bad anchor
 *        phrases via embedding similarity.
 *
 * Motivation
 * ----------
 * Hate speech detection by wordlist is a token-level match: any
 * single slur on the list fires SlurFilter. Paraphrases that don't
 * use any token on the list — sentence-level patterns like "we
 * should get rid of [ethnic group] permanently" — evade token
 * matching entirely while still being recognizable as hate speech.
 * The remote classifier catches them but at a cost. The local
 * linear classifier catches OBVIOUS cases but has mediocre recall
 * on rhetorical/indirect hate (AUC ~0.76 on the hate axis in the
 * trained weights).
 *
 * BadHintLayer is the counterpart of SafeHintLayer: it maintains a
 * small list of operator-curated "obviously bad" anchor phrases,
 * embeds them at startup, and at message time computes the max
 * cosine similarity against all anchors. A match above the
 * configured threshold triggers a skip_bad verdict and injects a
 * score into a configurable axis (default `hate`), which flows
 * through the heat ladder like any other axis score.
 *
 * This is the single most direct way to plug recall holes in the
 * local classifier: add example phrases to the anchors list, redeploy.
 * No retraining, no model adjustment — just curate the examples.
 *
 * What this layer is NOT
 * ----------------------
 *  - A replacement for the remote classifier. Messages that don't
 *    look like any bad anchor still flow through the rest of the
 *    Layer 2 decision tree (trust_bank, safe_hint, remote).
 *  - A content policy decision on its own. It stacks on top of Layer
 *    1 and the other Layer 2 shortcuts; they all contribute scores.
 *  - A training signal. The anchors list is static for the lifetime
 *    of the process (refreshed only on restart, same as safe-hint).
 *  - A slur wordlist. Anchors are PHRASE-level, not token-level.
 *    SlurFilter handles tokens; BadHintLayer handles utterances.
 *
 * Scope limits
 * ------------
 *  - Requires the embedding backend to be ready. Until the HF model
 *    has loaded, is_active() returns false and every query returns
 *    is_bad=false (so ContentModerator falls through to other
 *    skip reasons).
 *  - Operator-curated anchor list. Bad curation = false positives.
 *    Start small (20-30 anchors) and iterate based on observed rate.
 *  - Threshold defaults HIGHER (0.75) than SafeHintLayer's (0.7)
 *    because a false positive here CAUSES enforcement action. We
 *    want to skew conservative on the "bad" side.
 */
#pragma once

#include "moderation/ContentModerationConfig.h"
#include "moderation/EmbeddingBackend.h"

#include <cstddef>
#include <shared_mutex>
#include <string>
#include <vector>

namespace moderation {

struct BadHintResult {
    bool is_bad = false;         ///< True iff max_similarity >= threshold.
    double max_similarity = 0.0; ///< [0, 1] cosine against nearest anchor.
    int best_anchor_index = -1;  ///< Index of the matching anchor, -1 on miss.
};

class BadHintLayer {
  public:
    BadHintLayer() = default;

    /// Scalar configuration only (threshold, enabled flag, inject_axis).
    /// Does NOT load anchors — call load_anchors() separately from the
    /// main.cpp background thread once both the embedding backend is
    /// ready AND the TextListFetcher has returned the anchor list.
    void configure(const BadHintConfig& cfg);

    /// Compute and install anchor embeddings. Takes the raw anchor
    /// strings and a borrowed backend pointer. Embeds each anchor
    /// once, stores the resulting unit vector, and drops the backend
    /// reference (the hot path uses the caller-supplied embedding).
    ///
    /// Entries that fail to embed are silently dropped — a typo or
    /// too-long entry shouldn't disable the whole layer.
    size_t load_anchors(const std::vector<std::string>& raw, EmbeddingBackend& backend);

    /// True if the layer is configured AND anchors are loaded.
    bool is_active() const;

    /// Number of anchors currently loaded.
    size_t anchor_count() const;

    /// Hot path: compare an already-computed message embedding
    /// against all stored anchors and return the max similarity.
    /// BadHintLayer takes an EmbeddingResult directly (not a
    /// backend) because ContentModerator always computes the
    /// embedding once and shares it across SafeHint/LocalClassifier/
    /// BadHint — there's no reason to re-embed here.
    ///
    /// Failure modes — returns is_bad=false when:
    ///   - no anchors loaded
    ///   - @p embedding.ok is false
    ///   - embedding dim != anchor dim
    BadHintResult query_with_embedding(const EmbeddingResult& embedding) const;

    /// The configured injection axis. ContentModerator reads this
    /// to decide which score field to bump on a hit.
    const std::string& inject_axis() const;

    /// The configured injection score. ContentModerator writes this
    /// into the named axis on a hit.
    double inject_score() const;

  private:
    mutable std::shared_mutex mu_;
    BadHintConfig cfg_{};

    // Flat row-major anchor storage, same layout as SafeHintLayer.
    std::vector<float> anchors_flat_;
    int anchor_dim_ = 0;
    size_t anchor_rows_ = 0;
};

} // namespace moderation
