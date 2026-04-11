#include "moderation/BadHintLayer.h"

#include "utils/Log.h"

// std::unique_lock is in <mutex>, not <shared_mutex>. libc++ pulls it
// transitively but libstdc++ on Linux does not — an explicit include
// here is required to keep CI green. Same footgun as SafeHintLayer.
#include <mutex>

namespace moderation {

void BadHintLayer::configure(const BadHintConfig& cfg) {
    std::unique_lock lock(mu_);
    cfg_ = cfg;
    // Same policy as SafeHintLayer: a reconfigure that only changes
    // scalars (threshold, inject_axis) should leave already-loaded
    // anchors in place. The operator doesn't need to also call
    // load_anchors() to apply a threshold bump.
}

size_t BadHintLayer::load_anchors(const std::vector<std::string>& raw, EmbeddingBackend& backend) {
    if (!backend.is_ready()) {
        Log::log_print(WARNING, "BadHintLayer: load_anchors called but backend is not ready; skipping");
        return 0;
    }
    const int dim = backend.dimension();
    if (dim <= 0) {
        Log::log_print(WARNING, "BadHintLayer: backend reports zero dimension; skipping");
        return 0;
    }

    // Embed first, swap under lock. Same reasoning as SafeHintLayer:
    // embedding is slow and we don't want every check() call to
    // serialize on the anchor-reload critical section.
    std::vector<float> fresh;
    fresh.reserve(static_cast<size_t>(dim) * raw.size());
    size_t ok_count = 0;
    for (const auto& anchor : raw) {
        if (anchor.empty())
            continue;
        auto result = backend.embed(anchor);
        if (!result.ok) {
            Log::log_print(WARNING, "BadHintLayer: anchor embed failed (%s)", result.error.c_str());
            continue;
        }
        if (static_cast<int>(result.vector.size()) != dim) {
            Log::log_print(WARNING, "BadHintLayer: dim mismatch (%d vs %d); skipping anchor",
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

bool BadHintLayer::is_active() const {
    std::shared_lock lock(mu_);
    return cfg_.enabled && anchor_rows_ > 0;
}

size_t BadHintLayer::anchor_count() const {
    std::shared_lock lock(mu_);
    return anchor_rows_;
}

BadHintResult BadHintLayer::query_with_embedding(const EmbeddingResult& embedding) const {
    BadHintResult out;

    if (!embedding.ok)
        return out;

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

    if (static_cast<int>(embedding.vector.size()) != dim)
        return out;

    // Max cosine similarity = max dot product since both sides are
    // L2-normalized. Compiler autovectorizes the inner loop.
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
    out.is_bad = (max_sim >= threshold);
    return out;
}

const std::string& BadHintLayer::inject_axis() const {
    std::shared_lock lock(mu_);
    return cfg_.inject_axis;
}

double BadHintLayer::inject_score() const {
    std::shared_lock lock(mu_);
    return cfg_.inject_score;
}

} // namespace moderation
