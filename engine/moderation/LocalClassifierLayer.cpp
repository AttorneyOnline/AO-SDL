#include "moderation/LocalClassifierLayer.h"

#include "utils/Log.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace moderation {

namespace {

constexpr const char kMagicPrefix[] = "KGCLF";

// V2 header: magic(8) + version(4) + num_cat(4) + dim(4) + hidden_dim(4) + name_len(4)
constexpr size_t kV2HeaderFixedSize = 8 + 4 + 4 + 4 + 4 + 4;

uint32_t read_u32_le(const uint8_t* data, size_t off) {
    uint32_t v = 0;
    v |= static_cast<uint32_t>(data[off + 0]);
    v |= static_cast<uint32_t>(data[off + 1]) << 8;
    v |= static_cast<uint32_t>(data[off + 2]) << 16;
    v |= static_cast<uint32_t>(data[off + 3]) << 24;
    return v;
}

double sigmoid(double x) {
    if (x >= 0.0) {
        const double z = std::exp(-x);
        return 1.0 / (1.0 + z);
    }
    const double z = std::exp(x);
    return z / (1.0 + z);
}

/// Assign a probability to the appropriate axis field by category index.
/// Canonical order from scripts/train_classifier.py:
///   0=hate 1=sexual 2=sexual_minors 3=violence 4=self_harm
///   5=visual_noise 6=link_risk (sentinel-biased, effectively unused)
void set_axis(ModerationAxisScores& scores, int index, double prob) {
    switch (index) {
    case 0:
        scores.hate = prob;
        break;
    case 1:
        scores.sexual = prob;
        break;
    case 2:
        scores.sexual_minors = prob;
        break;
    case 3:
        scores.violence = prob;
        break;
    case 4:
        scores.self_harm = prob;
        break;
    case 5:
        scores.visual_noise = prob;
        break;
    case 6:
        scores.link_risk = prob;
        break;
    default:
        break;
    }
}

/// Check if the runtime and file embedding model repos match.
/// Strips colon-suffixed quantization filenames before comparing.
bool model_name_matches(const std::string& file_name, const std::string& runtime_name) {
    if (runtime_name.empty())
        return true; // no runtime check requested
    auto strip = [](const std::string& s) -> std::string {
        auto colon = s.find(':');
        return colon == std::string::npos ? s : s.substr(0, colon);
    };
    return strip(file_name) == strip(runtime_name);
}

} // namespace

void LocalClassifierLayer::configure(const LocalClassifierConfig& cfg) {
    std::unique_lock lock(mu_);
    cfg_ = cfg;
}

bool LocalClassifierLayer::load_weights(const uint8_t* blob, size_t blob_size, const std::string& runtime_model_name) {
    if (blob == nullptr || blob_size < kV2HeaderFixedSize) {
        Log::warn("LocalClassifier: weights blob missing or too short ({} bytes)", blob_size);
        {
            std::unique_lock lock(mu_);
            weights_ = nullptr;
        }
        return false;
    }

    if (std::memcmp(blob, kMagicPrefix, 5) != 0) {
        Log::warn("LocalClassifier: magic prefix mismatch");
        {
            std::unique_lock lock(mu_);
            weights_ = nullptr;
        }
        return false;
    }

    const uint8_t magic_version = blob[5];
    if (magic_version != 0x02) {
        Log::warn("LocalClassifier: unsupported magic version byte 0x{:02x}", magic_version);
        {
            std::unique_lock lock(mu_);
            weights_ = nullptr;
        }
        return false;
    }

    const uint32_t version = read_u32_le(blob, 8);
    const uint32_t num_cat = read_u32_le(blob, 12);
    const uint32_t dim = read_u32_le(blob, 16);

    if (num_cat == 0 || num_cat > 64) {
        Log::warn("LocalClassifier: num_categories out of range: {}", num_cat);
        {
            std::unique_lock lock(mu_);
            weights_ = nullptr;
        }
        return false;
    }
    if (dim == 0 || dim > 4096) {
        Log::warn("LocalClassifier: embedding_dim out of range: {}", dim);
        {
            std::unique_lock lock(mu_);
            weights_ = nullptr;
        }
        return false;
    }
    if (version != 2) {
        Log::warn("LocalClassifier: unsupported format version {}", version);
        {
            std::unique_lock lock(mu_);
            weights_ = nullptr;
        }
        return false;
    }

    const uint32_t hidden = read_u32_le(blob, 20);
    const uint32_t name_len = read_u32_le(blob, 24);
    if (hidden == 0 || hidden > 1024) {
        Log::warn("LocalClassifier: hidden_dim out of range: {}", hidden);
        {
            std::unique_lock lock(mu_);
            weights_ = nullptr;
        }
        return false;
    }
    if (name_len > 512) {
        Log::warn("LocalClassifier: model_name_len too long: {}", name_len);
        {
            std::unique_lock lock(mu_);
            weights_ = nullptr;
        }
        return false;
    }

    const size_t nc = num_cat, d = dim, h = hidden;
    const size_t w1_bytes = nc * d * h * sizeof(float);
    const size_t b1_bytes = nc * h * sizeof(float);
    const size_t w2_bytes = nc * h * sizeof(float);
    const size_t b2_bytes = nc * sizeof(float);
    const size_t wskip_bytes = nc * d * sizeof(float);
    const size_t min_size = kV2HeaderFixedSize + name_len + w1_bytes + b1_bytes + w2_bytes + b2_bytes + wskip_bytes + 1;
    if (blob_size < min_size) {
        Log::warn("LocalClassifier: blob too small {} vs minimum {}", blob_size, min_size);
        {
            std::unique_lock lock(mu_);
            weights_ = nullptr;
        }
        return false;
    }

    std::string file_model(reinterpret_cast<const char*>(blob + kV2HeaderFixedSize), name_len);
    if (!model_name_matches(file_model, runtime_model_name)) {
        Log::warn("LocalClassifier: model mismatch '{}' vs runtime '{}'", file_model, runtime_model_name);
        {
            std::unique_lock lock(mu_);
            weights_ = nullptr;
        }
        return false;
    }

    // Build an immutable Weights struct. Once published via atomic store,
    // it is shared across all classify() calls with no locking.
    auto w = std::make_shared<Weights>();
    w->num_categories = static_cast<int>(num_cat);
    w->embedding_dim = static_cast<int>(dim);
    w->hidden_dim = static_cast<int>(hidden);
    w->model_name = std::move(file_model);

    const uint8_t* cursor = blob + kV2HeaderFixedSize + name_len;

    // Read W1 from disk as [num_cat × dim × hidden] and transpose to
    // [num_cat × hidden × dim] for contiguous inner-loop access.
    // On disk: w1_disk[i*D*H + j*H + k] = W1[axis=i, input=j, hidden=k]
    // In memory: w1[i*H*D + k*D + j] = W1[axis=i, hidden=k, input=j]
    {
        std::vector<float> w1_disk(nc * d * h);
        std::memcpy(w1_disk.data(), cursor, w1_bytes);
        cursor += w1_bytes;

        w->w1.resize(nc * h * d);
        for (size_t i = 0; i < nc; ++i)
            for (size_t k = 0; k < h; ++k)
                for (size_t j = 0; j < d; ++j)
                    w->w1[i * h * d + k * d + j] = w1_disk[i * d * h + j * h + k];
    }

    w->b1.resize(nc * h);
    std::memcpy(w->b1.data(), cursor, b1_bytes);
    cursor += b1_bytes;

    w->w2.resize(nc * h);
    std::memcpy(w->w2.data(), cursor, w2_bytes);
    cursor += w2_bytes;

    w->b2.resize(nc);
    std::memcpy(w->b2.data(), cursor, b2_bytes);
    cursor += b2_bytes;

    w->w_skip.resize(nc * d);
    std::memcpy(w->w_skip.data(), cursor, wskip_bytes);
    cursor += wskip_bytes;

    w->calibration_type = static_cast<int>(*cursor);
    cursor += 1;

    if (w->calibration_type == 1) {
        const size_t platt_bytes = nc * sizeof(float);
        if (blob_size < static_cast<size_t>(cursor - blob) + 2 * platt_bytes) {
            Log::warn("LocalClassifier: blob too small for Platt calibration data");
            {
                std::unique_lock lock(mu_);
                weights_ = nullptr;
            }
            return false;
        }
        w->platt_a.resize(nc);
        std::memcpy(w->platt_a.data(), cursor, platt_bytes);
        cursor += platt_bytes;
        w->platt_b.resize(nc);
        std::memcpy(w->platt_b.data(), cursor, platt_bytes);
    }

    // Publish under unique_lock — all subsequent classify() calls
    // (which take shared_lock) see the new weights.
    {
        std::unique_lock lock(mu_);
        weights_ = std::move(w);
    }

    // Safe to read through the local w — we moved ownership into
    // weights_ but shared_ptr move leaves w null. Snapshot again.
    auto published = [&] {
        std::shared_lock lock(mu_);
        return weights_;
    }();

    Log::info("LocalClassifier: loaded v2 MLP {} categories x {}-dim -> {}-hidden "
              "(model={}, calibration={})",
              num_cat, dim, hidden, published->model_name, published->calibration_type == 1 ? "platt" : "none");
    return true;
}

bool LocalClassifierLayer::is_active() const {
    std::shared_lock lock(mu_);
    return cfg_.enabled && weights_ != nullptr;
}

int LocalClassifierLayer::num_categories() const {
    std::shared_lock lock(mu_);
    return weights_ ? weights_->num_categories : 0;
}

int LocalClassifierLayer::embedding_dim() const {
    std::shared_lock lock(mu_);
    return weights_ ? weights_->embedding_dim : 0;
}

const std::string& LocalClassifierLayer::model_name() const {
    static const std::string empty;
    std::shared_lock lock(mu_);
    return weights_ ? weights_->model_name : empty;
}

LocalClassifierResult LocalClassifierLayer::classify(const EmbeddingResult& embedding) const {
    // Snapshot the weights pointer under shared_lock. Multiple classify()
    // calls run concurrently — shared_lock doesn't serialize readers.
    // load_weights() takes unique_lock and blocks until readers finish.
    std::shared_ptr<const Weights> w;
    {
        std::shared_lock lock(mu_);
        if (!cfg_.enabled)
            return {};
        w = weights_;
    }
    if (!w)
        return {};

    if (!embedding.ok)
        return {};
    if (static_cast<int>(embedding.vector.size()) != w->embedding_dim)
        return {};

    return classify_impl(*w, embedding);
}

LocalClassifierResult LocalClassifierLayer::classify_impl(const Weights& w, const EmbeddingResult& embedding) {
    // Per-axis MLP with residual skip:
    //   hidden = ReLU(W1[i] · x + b1[i])
    //   logit  = W2[i] · hidden + b2[i] + W_skip[i] · x
    //   prob   = calibrate(logit)
    //
    // W1 is stored as [num_cat × hidden × dim] (transposed from disk
    // format) so the inner dot-product loop is a contiguous read.

    LocalClassifierResult out;
    out.max_confidence = 0.0;
    out.max_category_index = -1;

    const float* x = embedding.vector.data();
    const int D = w.embedding_dim;
    const int H = w.hidden_dim;

    // Heap-allocated hidden buffer — hidden_dim is runtime-configured.
    std::vector<double> hidden(H);

    for (int i = 0; i < w.num_categories; ++i) {
        // --- Layer 1: x → hidden ---
        // W1 transposed: w1[i*H*D + k*D + j] = W1[axis=i, hidden=k, input=j]
        // Inner loop over j is a contiguous dot product for each hidden unit k.
        const float* w1_axis = w.w1.data() + static_cast<size_t>(i) * H * D;
        const float* b1_axis = w.b1.data() + static_cast<size_t>(i) * H;
        for (int k = 0; k < H; ++k) {
            const float* w1_row = w1_axis + static_cast<size_t>(k) * D;
            double val = b1_axis[k];
            for (int j = 0; j < D; ++j)
                val += static_cast<double>(w1_row[j]) * static_cast<double>(x[j]);
            hidden[k] = std::max(0.0, val); // ReLU
        }

        // --- Layer 2: hidden → logit, plus residual skip ---
        const float* w2_axis = w.w2.data() + static_cast<size_t>(i) * H;
        const float b2_val = w.b2[i];
        const float* wskip_axis = w.w_skip.data() + static_cast<size_t>(i) * D;

        double logit = b2_val;
        for (int k = 0; k < H; ++k)
            logit += static_cast<double>(w2_axis[k]) * hidden[k];
        // Residual: W_skip[i] · x
        for (int j = 0; j < D; ++j)
            logit += static_cast<double>(wskip_axis[j]) * static_cast<double>(x[j]);

        // --- Calibration ---
        double prob;
        if (w.calibration_type == 1 && static_cast<size_t>(i) < w.platt_a.size()) {
            prob = sigmoid(static_cast<double>(w.platt_a[i]) * logit + static_cast<double>(w.platt_b[i]));
        }
        else {
            prob = sigmoid(logit);
        }

        set_axis(out.scores, i, prob);
        if (prob > out.max_confidence) {
            out.max_confidence = prob;
            out.max_category_index = i;
        }
    }

    out.ran = true;
    return out;
}

} // namespace moderation
