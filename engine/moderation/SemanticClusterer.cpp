#include "moderation/SemanticClusterer.h"

#include "metrics/MetricsRegistry.h"
#include "utils/Log.h"

#include <algorithm>
#include <chrono>
#include <utility>

namespace moderation {

void SemanticClusterer::configure(const EmbeddingsLayerConfig& cfg) {
    std::lock_guard lock(mu_);
    cfg_ = cfg;
    // Shrinking the window deletes excess history immediately; growing
    // keeps old entries (prune on sweep handles them).
    while (ring_.size() > static_cast<size_t>(std::max(1, cfg_.ring_size)))
        ring_.pop_front();
}

void SemanticClusterer::set_backend(std::unique_ptr<EmbeddingBackend> backend) {
    std::lock_guard lock(mu_);
    backend_ = std::move(backend);
}

int64_t SemanticClusterer::now_ms() const {
    if (clock_)
        return clock_();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

double SemanticClusterer::cosine(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty())
        return 0.0;
    double dot = 0.0;
    for (size_t i = 0; i < a.size(); ++i)
        dot += static_cast<double>(a[i]) * static_cast<double>(b[i]);
    return dot;
}

void SemanticClusterer::prune_locked(int64_t now_ms) {
    const int64_t window_ms = static_cast<int64_t>(cfg_.window_seconds) * 1000;
    const int64_t cutoff = now_ms - window_ms;
    while (!ring_.empty() && ring_.front().timestamp_ms < cutoff)
        ring_.pop_front();
    while (ring_.size() > static_cast<size_t>(std::max(1, cfg_.ring_size)))
        ring_.pop_front();
}

SemanticClusterResult SemanticClusterer::score(const std::string& ipid, std::string_view message) {
    // Metric families. Static locals so registration happens once per
    // process and the hot path is just an atomic increment.
    static auto& embed_tokens = metrics::MetricsRegistry::instance().counter(
        "kagami_moderation_embedding_tokens_total",
        "Total tokens processed by the embedding backend");
    static auto& embed_tokenize_ns = metrics::MetricsRegistry::instance().counter(
        "kagami_moderation_embedding_tokenize_nanoseconds_total",
        "Wall-clock nanoseconds spent tokenizing inputs for the embedding backend");
    static auto& embed_decode_ns = metrics::MetricsRegistry::instance().counter(
        "kagami_moderation_embedding_decode_nanoseconds_total",
        "Wall-clock nanoseconds spent in llama_decode (model forward pass) for embeddings");
    static auto& embed_errors = metrics::MetricsRegistry::instance().counter(
        "kagami_moderation_embedding_errors_total",
        "Embedding backend failures by reason", {"reason"});
    static auto& cluster_fires = metrics::MetricsRegistry::instance().counter(
        "kagami_moderation_semantic_cluster_fires_total",
        "Times the semantic cluster detector crossed the cluster_threshold");

    SemanticClusterResult r;
    if (!cfg_.enabled)
        return r;

    // Pull a copy of the backend pointer under the lock so the embed()
    // call happens without it. Backends may be slow (tens to hundreds
    // of ms for large models) and we don't want to block the ring
    // buffer for other threads during inference.
    EmbeddingBackend* backend = nullptr;
    {
        std::lock_guard lock(mu_);
        backend = backend_.get();
    }
    if (!backend || !backend->is_ready()) {
        embed_errors.labels({"not_ready"}).inc();
        return r;
    }

    auto er = backend->embed(message);
    if (!er.ok || er.vector.empty()) {
        const std::string reason = er.error.empty() ? "unknown"
                                   : er.error.find("empty") != std::string::npos ? "empty_text"
                                   : er.error.find("tokenize") != std::string::npos ? "tokenize"
                                   : er.error.find("decode") != std::string::npos ? "decode"
                                                                                   : "other";
        embed_errors.labels({reason}).inc();
        return r;
    }

    // Per-call stats from the backend.
    embed_tokens.get().inc(static_cast<uint64_t>(er.token_count));
    embed_tokenize_ns.get().inc(static_cast<uint64_t>(er.tokenize_ns));
    embed_decode_ns.get().inc(static_cast<uint64_t>(er.decode_ns));

    // Insert + compare under the lock. Scan the ring for near-dupes
    // and count distinct IPIDs in the match set, excluding self.
    std::unordered_set<std::string> distinct_others;
    int matched = 0;
    const int64_t t = now_ms();
    {
        std::lock_guard lock(mu_);
        prune_locked(t);

        for (auto& entry : ring_) {
            if (entry.vector.size() != er.vector.size())
                continue;
            double sim = cosine(er.vector, entry.vector);
            if (sim >= cfg_.similarity_threshold) {
                ++matched;
                if (entry.ipid != ipid)
                    distinct_others.insert(entry.ipid);
            }
        }

        // Insert this message into the ring AFTER scanning, so the
        // very message being checked doesn't count as its own dup.
        Entry e;
        e.timestamp_ms = t;
        e.ipid = ipid;
        e.vector = std::move(er.vector);
        ring_.push_back(std::move(e));
        while (ring_.size() > static_cast<size_t>(std::max(1, cfg_.ring_size)))
            ring_.pop_front();
    }

    r.matched_count = matched;
    r.distinct_ipids = static_cast<int>(distinct_others.size());

    // Fire the signal only when we've seen the near-dupe from at
    // least cluster_threshold distinct IPIDs. Single-source echoes
    // are already handled by SpamDetector H1.
    if (r.distinct_ipids + 1 >= cfg_.cluster_threshold) {
        const double excess = static_cast<double>(r.distinct_ipids + 1 - cfg_.cluster_threshold);
        r.score = std::clamp(1.0 + excess * 0.2, 0.0, 1.0);
        // clamp() above always returns 1.0 for excess>=0; the nudge is
        // only meaningful in future if we raise the ceiling above 1.0.
        if (r.distinct_ipids + 1 == cfg_.cluster_threshold)
            r.score = 1.0;
        r.reason = "semantic_cluster(" + std::to_string(r.distinct_ipids + 1) + "_ipids)";
        cluster_fires.get().inc();
    }
    return r;
}

void SemanticClusterer::sweep() {
    std::lock_guard lock(mu_);
    prune_locked(now_ms());
}

size_t SemanticClusterer::window_size() const {
    std::lock_guard lock(mu_);
    return ring_.size();
}

} // namespace moderation
