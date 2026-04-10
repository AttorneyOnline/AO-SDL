/**
 * @file SemanticClusterer.h
 * @brief Near-duplicate detection across messages from distinct IPIDs.
 *
 * This is Layer 3 of the moderation stack. Given a stream of messages,
 * each with an IPID and an embedding, it answers: "has this message
 * been seen in near-identical form from enough distinct IPIDs recently
 * to be considered a coordinated spam wave?"
 *
 * This is the one place where embeddings genuinely earn their keep —
 * the existing SpamDetector H1 catches exact-fingerprint echoes, but
 * a semantic check catches paraphrased variants ("buy gold now",
 * "cheap gold on sale", "gold for sale — limited time").
 *
 * Thread safety: all public methods are mutex-protected.
 */
#pragma once

#include "moderation/ContentModerationConfig.h"
#include "moderation/EmbeddingBackend.h"

#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace moderation {

struct SemanticClusterResult {
    double score = 0.0;        ///< 0-1 signal (cluster_count_above / cluster_threshold clamped to 1).
    int matched_count = 0;      ///< Total recent messages above similarity threshold.
    int distinct_ipids = 0;     ///< Count of distinct IPIDs in the match set.
    std::string reason;         ///< Short description for audit log.
};

class SemanticClusterer {
  public:
    SemanticClusterer() = default;

    void configure(const EmbeddingsLayerConfig& cfg);

    /// Inject the embedding backend. Ownership transferred. Use
    /// NullEmbeddingBackend if no ML model is available — all calls
    /// will return a zero score and the layer becomes a no-op.
    void set_backend(std::unique_ptr<EmbeddingBackend> backend);

    /// Score a new message. Side effect: inserts the embedding into
    /// the rolling window, so successive calls build up the history.
    /// Returns a zero result if the backend isn't ready or if the
    /// layer is disabled.
    SemanticClusterResult score(const std::string& ipid, std::string_view message);

    /// Periodic housekeeping: drop entries older than the window.
    void sweep();

    /// Count of entries currently in the window. For diagnostics.
    size_t window_size() const;

    /// For tests: override the clock source.
    using Clock = int64_t (*)();
    void set_clock_for_tests(Clock clock) {
        clock_ = clock;
    }

  private:
    struct Entry {
        int64_t timestamp_ms;
        std::string ipid;
        std::vector<float> vector;
    };

    int64_t now_ms() const;

    /// Cosine similarity on unit-normalized vectors = dot product.
    /// Expects both vectors to be the same size and unit-length.
    static double cosine(const std::vector<float>& a, const std::vector<float>& b);

    /// Drop entries older than the window. Caller holds mu_.
    void prune_locked(int64_t now_ms);

    EmbeddingsLayerConfig cfg_;
    std::unique_ptr<EmbeddingBackend> backend_;
    mutable std::mutex mu_;
    std::deque<Entry> ring_;
    Clock clock_ = nullptr;
};

} // namespace moderation
