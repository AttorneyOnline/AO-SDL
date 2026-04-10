/**
 * @file SafeHintLayer.h
 * @brief Layer 2 shortcut: skip the remote classifier on messages
 *        that are embedding-similar to a known-safe anchor phrase.
 *
 * Motivation
 * ----------
 * The remote classifier (Layer 2) is by far the most expensive part
 * of ContentModerator::check() — network round-trip, OpenAI per-token
 * cost, and the only layer that a per-IPID rate limit protects us
 * from. For the overwhelming majority of messages in a healthy chat
 * server ("hi", "brb", "gg", "what time is it") the classifier will
 * return zeros and we spent money on a foregone conclusion.
 *
 * The safe-hint shortcut cuts that waste. At startup we fetch a list
 * of ~50 "obviously safe" anchor phrases from an operator-supplied
 * S3 URL, run them through the local embedding model once, and
 * cache the unit-normalized vectors. On the hot path, we embed the
 * incoming message and take the max cosine similarity (= max dot
 * product, since both sides are L2-normalized) against all anchors.
 * If it exceeds the configured threshold (default 0.7), we skip
 * the OpenAI call entirely and record the skip reason for metrics.
 *
 * What this layer is NOT
 * ----------------------
 *  - A content policy decision. Safe-hint is a *cost* optimization,
 *    not a safety decision. Layer 1 (unicode/urls/slurs) still runs
 *    on every message regardless of safe-hint outcome. An abusive
 *    message that happens to embed-close to a safe anchor is still
 *    caught by the word-boundary slur filter.
 *  - A replacement for Layer 2. Messages that DON'T look like any
 *    safe anchor still go through the remote classifier normally.
 *  - A training signal. We don't ever update the anchors based on
 *    classifier output — the list is operator-curated and static
 *    for the lifetime of the process (refreshed only on restart).
 *
 * Scope limits
 * ------------
 *  - Requires the embedding backend to be ready. Until the HF model
 *    has loaded, is_ready() returns false and every query returns
 *    "not safe" (so the remote layer fires normally).
 *  - Runs its own embedding computation per message; does NOT share
 *    the vector that SemanticClusterer computes. This is a deliberate
 *    scope choice — plumbing a cached vector through the existing
 *    clusterer API would touch a lot of code for a ~2ms savings on
 *    the subset of messages where both layers are active. A future
 *    refactor can unify the two embedding calls.
 */
#pragma once

#include "moderation/ContentModerationConfig.h"
#include "moderation/EmbeddingBackend.h"

#include <cstddef>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <vector>

namespace moderation {

struct SafeHintResult {
    bool is_safe = false;        ///< True iff max_similarity >= threshold.
    double max_similarity = 0.0; ///< [0, 1] cosine against nearest anchor.
    int best_anchor_index = -1;  ///< Index of the matching anchor, -1 on miss.
};

class SafeHintLayer {
  public:
    SafeHintLayer() = default;

    /// Scalar configuration only (threshold, enabled flag). Does NOT
    /// load anchors — call load_anchors() separately from the main.cpp
    /// background thread once the embedding backend is ready AND the
    /// TextListFetcher has returned.
    void configure(const SafeHintConfig& cfg);

    /// Compute and install anchor embeddings. Takes both the raw
    /// anchor strings and a borrowed backend pointer. Embeds each
    /// anchor once, stores the resulting unit vector, and drops the
    /// backend reference (the hot path uses the caller-supplied
    /// backend for message embeddings).
    ///
    /// Entries that fail to embed are silently dropped — a typo or
    /// too-long entry in the anchor list shouldn't disable the layer.
    /// Returns the number of successfully-loaded anchors.
    size_t load_anchors(const std::vector<std::string>& raw, EmbeddingBackend& backend);

    /// True if the layer is configured AND anchors are loaded.
    /// ContentModerator skips the query entirely when this is false.
    bool is_active() const;

    /// Number of anchors currently loaded.
    size_t anchor_count() const;

    /// Hot path: embed @p message and return the max cosine similarity
    /// against all stored anchors. The caller passes in the backend
    /// because it's owned by SemanticClusterer/ContentModerator, not
    /// by this layer.
    ///
    /// Failure modes (all return is_safe=false, max_similarity=0):
    ///   - backend not ready
    ///   - no anchors loaded
    ///   - empty message
    ///   - embed() returned !ok
    SafeHintResult query(std::string_view message, EmbeddingBackend& backend) const;

  private:
    mutable std::shared_mutex mu_;

    // cfg_ is read under a shared lock on the hot path. We could
    // copy-on-configure like ContentModerator does, but SafeHintConfig
    // is tiny (two strings + two doubles + one bool) and contention
    // is essentially zero.
    SafeHintConfig cfg_{};

    // Each anchor is stored as a unit-normalized float vector. Layout
    // is flat (anchor_dim_ floats per anchor, concatenated) so the
    // dot-product loop is cache-friendly. A vector<vector<float>>
    // would double-allocate and hurt locality for no reason.
    std::vector<float> anchors_flat_;
    int anchor_dim_ = 0;
    size_t anchor_rows_ = 0;
};

} // namespace moderation
