#include "moderation/SafeHintLayer.h"

#include "utils/Log.h"

// std::unique_lock is in <mutex>, not <shared_mutex>. The header
// already pulls <shared_mutex> for the shared_lock used in query(),
// but libstdc++ doesn't transitively include <mutex> from there, so
// the load_anchors() / configure() unique_lock<shared_mutex> uses
// fail to compile on Linux without this explicit include. (libc++
// happens to pull it transitively, which is why the macOS local
// build was clean while CI Linux failed.)
#include <mutex>

namespace moderation {

void SafeHintLayer::configure(const SafeHintConfig& cfg) {
    std::unique_lock lock(mu_);
    cfg_ = cfg;
    // Do NOT clear anchors_ here. A runtime reconfigure that only
    // changes the threshold or the URL (without a corresponding
    // load_anchors call) should keep the already-loaded vectors
    // live — same pattern as SlurFilter::configure().
}

size_t SafeHintLayer::load_anchors(const std::vector<std::string>& raw, EmbeddingBackend& backend) {
    if (!backend.is_ready()) {
        Log::log_print(WARNING, "SafeHintLayer: load_anchors called but backend is not ready; skipping");
        return 0;
    }
    const int dim = backend.dimension();
    if (dim <= 0) {
        Log::log_print(WARNING, "SafeHintLayer: backend reports zero dimension; skipping");
        return 0;
    }

    // Compute embeddings first, then swap under the lock. This keeps
    // the critical section short even if the embedding model is slow
    // (~5ms per anchor on bge-small is 250ms for 50 anchors, which
    // we do NOT want to hold a lock across).
    std::vector<float> fresh;
    fresh.reserve(static_cast<size_t>(dim) * raw.size());
    size_t ok_count = 0;
    for (const auto& anchor : raw) {
        if (anchor.empty())
            continue;
        auto result = backend.embed(anchor);
        if (!result.ok) {
            Log::log_print(WARNING, "SafeHintLayer: anchor embed failed (%s): %s", result.error.c_str(),
                           anchor.c_str());
            continue;
        }
        if (static_cast<int>(result.vector.size()) != dim) {
            // Dimension mismatch — shouldn't happen with a sane
            // backend, but a guard here is cheap and saves debugging
            // a confusing dot-product crash later.
            Log::log_print(WARNING, "SafeHintLayer: dim mismatch (%d vs %d); skipping anchor",
                           static_cast<int>(result.vector.size()), dim);
            continue;
        }
        fresh.insert(fresh.end(), result.vector.begin(), result.vector.end());
        ++ok_count;
    }

    std::unique_lock lock(mu_);
    anchors_flat_ = std::move(fresh);
    anchor_dim_ = dim;
    anchor_rows_ = ok_count;
    return ok_count;
}

bool SafeHintLayer::is_active() const {
    std::shared_lock lock(mu_);
    return cfg_.enabled && anchor_rows_ > 0;
}

size_t SafeHintLayer::anchor_count() const {
    std::shared_lock lock(mu_);
    return anchor_rows_;
}

SafeHintResult SafeHintLayer::query(std::string_view message, EmbeddingBackend& backend) const {
    SafeHintResult out;

    if (message.empty())
        return out;
    if (!backend.is_ready())
        return out;

    // Snapshot the state we need under the shared lock, then release.
    // The actual dot-product loop runs lock-free against the snapshot
    // — configure()/load_anchors() can't invalidate it mid-scan
    // because the only way to grow anchors_flat_ is through a unique
    // lock we held off until now.
    int dim = 0;
    size_t rows = 0;
    double threshold = 0.0;
    bool enabled = false;
    std::vector<float> snapshot;
    {
        std::shared_lock lock(mu_);
        enabled = cfg_.enabled;
        threshold = cfg_.similarity_threshold;
        dim = anchor_dim_;
        rows = anchor_rows_;
        if (!enabled || rows == 0 || dim <= 0)
            return out;
        // We copy here rather than holding the lock during embed()
        // because embed() is expensive and we don't want to serialize
        // message processing on the anchor reloader. A 50-anchor ×
        // 384-dim snapshot is ~75 KB — cheap to copy, cheap to free.
        snapshot = anchors_flat_;
    }

    // Embed the message. This is the expensive call (typically 1-5ms
    // on bge-small CPU). Runs without the lock held so two threads
    // can query() concurrently.
    auto result = backend.embed(message);
    if (!result.ok)
        return out;
    if (static_cast<int>(result.vector.size()) != dim)
        return out;

    // Max cosine similarity = max dot product, because both the
    // anchors and the message embedding are L2-normalized by the
    // backend contract. Scanning row-by-row with a tight inner
    // loop the compiler can vectorize.
    double max_sim = -1.0;
    int best = -1;
    const float* msg = result.vector.data();
    for (size_t r = 0; r < rows; ++r) {
        const float* anchor = &snapshot[r * static_cast<size_t>(dim)];
        double dot = 0.0;
        for (int k = 0; k < dim; ++k)
            dot += static_cast<double>(anchor[k]) * static_cast<double>(msg[k]);
        if (dot > max_sim) {
            max_sim = dot;
            best = static_cast<int>(r);
        }
    }

    out.max_similarity = max_sim;
    out.best_anchor_index = best;
    out.is_safe = (max_sim >= threshold);
    return out;
}

SafeHintResult SafeHintLayer::query_with_embedding(const EmbeddingResult& embedding) const {
    SafeHintResult out;

    if (!embedding.ok)
        return out;

    // Snapshot under the shared lock like the message-based overload.
    // See that function for the reason this copies instead of holding
    // the lock through the dot-product loop.
    int dim = 0;
    size_t rows = 0;
    double threshold = 0.0;
    std::vector<float> snapshot;
    {
        std::shared_lock lock(mu_);
        if (!cfg_.enabled || anchor_rows_ == 0 || anchor_dim_ <= 0)
            return out;
        dim = anchor_dim_;
        rows = anchor_rows_;
        threshold = cfg_.similarity_threshold;
        snapshot = anchors_flat_;
    }

    // Embedding dim must match the anchors. If the caller handed us
    // a vector from a different backend we'd produce garbage, so
    // fail-closed.
    if (static_cast<int>(embedding.vector.size()) != dim)
        return out;

    double max_sim = -1.0;
    int best = -1;
    const float* msg = embedding.vector.data();
    for (size_t r = 0; r < rows; ++r) {
        const float* anchor = &snapshot[r * static_cast<size_t>(dim)];
        double dot = 0.0;
        for (int k = 0; k < dim; ++k)
            dot += static_cast<double>(anchor[k]) * static_cast<double>(msg[k]);
        if (dot > max_sim) {
            max_sim = dot;
            best = static_cast<int>(r);
        }
    }

    out.max_similarity = max_sim;
    out.best_anchor_index = best;
    out.is_safe = (max_sim >= threshold);
    return out;
}

} // namespace moderation
