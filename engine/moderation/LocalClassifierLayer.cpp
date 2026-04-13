#include "moderation/LocalClassifierLayer.h"

#include "utils/Log.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace moderation {

namespace {

constexpr const char kMagicPrefix[] = "KGCLF";
constexpr size_t kMagicTotalSize = 8;

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
    std::lock_guard lock(mu_);
    cfg_ = cfg;
}

bool LocalClassifierLayer::load_weights(const uint8_t* blob, size_t blob_size, const std::string& runtime_model_name) {
    std::lock_guard lock(mu_);
    loaded_ = false;
    w1_.clear();
    b1_.clear();
    w2_.clear();
    b2_.clear();
    w_skip_.clear();
    platt_a_.clear();
    platt_b_.clear();
    num_categories_ = 0;
    embedding_dim_ = 0;
    hidden_dim_ = 0;
    model_name_.clear();
    calibration_type_ = 0;

    if (blob == nullptr || blob_size < kV2HeaderFixedSize) {
        Log::warn("LocalClassifier: weights blob missing or too short ({} bytes)", blob_size);
        return false;
    }

    if (std::memcmp(blob, kMagicPrefix, 5) != 0) {
        Log::warn("LocalClassifier: magic prefix mismatch");
        return false;
    }

    const uint8_t magic_version = blob[5];
    if (magic_version != 0x02) {
        Log::warn("LocalClassifier: unsupported magic version byte 0x{:02x}", magic_version);
        return false;
    }

    const uint32_t version = read_u32_le(blob, 8);
    const uint32_t num_cat = read_u32_le(blob, 12);
    const uint32_t dim = read_u32_le(blob, 16);

    if (num_cat == 0 || num_cat > 64) {
        Log::warn("LocalClassifier: num_categories out of range: {}", num_cat);
        return false;
    }
    if (dim == 0 || dim > 4096) {
        Log::warn("LocalClassifier: embedding_dim out of range: {}", dim);
        return false;
    }

    if (version != 2) {
        Log::warn("LocalClassifier: unsupported format version {}", version);
        return false;
    }

    const uint32_t hidden = read_u32_le(blob, 20);
    const uint32_t name_len = read_u32_le(blob, 24);
    if (hidden == 0 || hidden > 1024) {
        Log::warn("LocalClassifier: hidden_dim out of range: {}", hidden);
        return false;
    }
    return load_weights_v2(blob, blob_size, num_cat, dim, name_len, runtime_model_name);
}

bool LocalClassifierLayer::load_weights_v2(const uint8_t* blob, size_t blob_size, uint32_t num_cat, uint32_t dim,
                                           uint32_t name_len, const std::string& runtime_model_name) {
    const uint32_t hidden = read_u32_le(blob, 20);
    if (name_len > 512) {
        Log::warn("LocalClassifier v2: model_name_len too long: {}", name_len);
        return false;
    }

    // Compute expected sizes for all weight tensors:
    //   W1: num_cat * dim * hidden
    //   b1: num_cat * hidden
    //   W2: num_cat * hidden
    //   b2: num_cat
    //   W_skip: num_cat * dim
    const size_t nc = num_cat, d = dim, h = hidden;
    const size_t w1_bytes = nc * d * h * sizeof(float);
    const size_t b1_bytes = nc * h * sizeof(float);
    const size_t w2_bytes = nc * h * sizeof(float);
    const size_t b2_bytes = nc * sizeof(float);
    const size_t wskip_bytes = nc * d * sizeof(float);
    const size_t min_size = kV2HeaderFixedSize + name_len + w1_bytes + b1_bytes + w2_bytes + b2_bytes + wskip_bytes +
                            1; // +1 for calibration_type
    if (blob_size < min_size) {
        Log::warn("LocalClassifier v2: blob too small {} vs minimum {}", blob_size, min_size);
        return false;
    }

    std::string file_model(reinterpret_cast<const char*>(blob + kV2HeaderFixedSize), name_len);
    if (!model_name_matches(file_model, runtime_model_name)) {
        Log::warn("LocalClassifier v2: model mismatch '{}' vs runtime '{}'", file_model, runtime_model_name);
        return false;
    }

    const uint8_t* cursor = blob + kV2HeaderFixedSize + name_len;

    w1_.resize(nc * d * h);
    std::memcpy(w1_.data(), cursor, w1_bytes);
    cursor += w1_bytes;

    b1_.resize(nc * h);
    std::memcpy(b1_.data(), cursor, b1_bytes);
    cursor += b1_bytes;

    w2_.resize(nc * h);
    std::memcpy(w2_.data(), cursor, w2_bytes);
    cursor += w2_bytes;

    b2_.resize(nc);
    std::memcpy(b2_.data(), cursor, b2_bytes);
    cursor += b2_bytes;

    w_skip_.resize(nc * d);
    std::memcpy(w_skip_.data(), cursor, wskip_bytes);
    cursor += wskip_bytes;

    calibration_type_ = static_cast<int>(*cursor);
    cursor += 1;

    if (calibration_type_ == 1) {
        const size_t platt_bytes = nc * sizeof(float);
        if (blob_size < static_cast<size_t>(cursor - blob) + 2 * platt_bytes) {
            Log::warn("LocalClassifier v2: blob too small for Platt calibration data");
            return false;
        }
        platt_a_.resize(nc);
        std::memcpy(platt_a_.data(), cursor, platt_bytes);
        cursor += platt_bytes;
        platt_b_.resize(nc);
        std::memcpy(platt_b_.data(), cursor, platt_bytes);
    }

    num_categories_ = static_cast<int>(num_cat);
    embedding_dim_ = static_cast<int>(dim);
    hidden_dim_ = static_cast<int>(hidden);
    model_name_ = std::move(file_model);
    loaded_ = true;
    Log::info("LocalClassifier: loaded v2 MLP {} categories x {}-dim → {}-hidden "
              "(model={}, calibration={})",
              num_categories_, embedding_dim_, hidden_dim_, model_name_, calibration_type_ == 1 ? "platt" : "none");
    return true;
}

bool LocalClassifierLayer::is_active() const {
    std::lock_guard lock(mu_);
    return cfg_.enabled && loaded_;
}

int LocalClassifierLayer::num_categories() const {
    std::lock_guard lock(mu_);
    return num_categories_;
}

int LocalClassifierLayer::embedding_dim() const {
    std::lock_guard lock(mu_);
    return embedding_dim_;
}

const std::string& LocalClassifierLayer::model_name() const {
    std::lock_guard lock(mu_);
    return model_name_;
}

LocalClassifierResult LocalClassifierLayer::classify(const EmbeddingResult& embedding) const {
    std::lock_guard lock(mu_);
    if (!loaded_ || !cfg_.enabled)
        return {};
    if (!embedding.ok)
        return {};
    if (static_cast<int>(embedding.vector.size()) != embedding_dim_)
        return {};

    return classify_v2(embedding);
}

LocalClassifierResult LocalClassifierLayer::classify_v2(const EmbeddingResult& embedding) const {
    // mu_ is already held by classify().
    //
    // Per-axis MLP with residual skip:
    //   hidden = ReLU(W1[i] · x + b1[i])
    //   logit  = W2[i] · hidden + b2[i] + W_skip[i] · x
    //   prob   = calibrate(logit)
    //
    // All matrix storage is row-major with axes as the leading dimension:
    //   W1: [num_cat × dim × hidden], accessed as w1_[i*dim*hidden + j*hidden + k]
    //   b1: [num_cat × hidden]
    //   W2: [num_cat × hidden]
    //   b2: [num_cat]
    //   W_skip: [num_cat × dim]

    LocalClassifierResult out;
    out.max_confidence = 0.0;
    out.max_category_index = -1;

    const float* x = embedding.vector.data();
    const int D = embedding_dim_;
    const int H = hidden_dim_;

    // Stack-allocate hidden layer buffer. Hidden dim is always small (≤1024,
    // typically 64), so this is safe on any reasonable stack.
    // Use heap for safety since hidden_dim is runtime-configured.
    std::vector<double> hidden(H);

    for (int i = 0; i < num_categories_; ++i) {
        // --- Layer 1: x → hidden ---
        // W1 for axis i starts at w1_[i * D * H], stored as [D rows × H cols] (row-major).
        // hidden[k] = b1[i*H + k] + sum_j(W1[i*D*H + j*H + k] * x[j])
        const float* w1_axis = w1_.data() + static_cast<size_t>(i) * D * H;
        const float* b1_axis = b1_.data() + static_cast<size_t>(i) * H;
        for (int k = 0; k < H; ++k) {
            double val = b1_axis[k];
            for (int j = 0; j < D; ++j)
                val += static_cast<double>(w1_axis[j * H + k]) * static_cast<double>(x[j]);
            hidden[k] = std::max(0.0, val); // ReLU
        }

        // --- Layer 2: hidden → logit, plus residual skip ---
        const float* w2_axis = w2_.data() + static_cast<size_t>(i) * H;
        const float b2_val = b2_[i];
        const float* wskip_axis = w_skip_.data() + static_cast<size_t>(i) * D;

        double logit = b2_val;
        for (int k = 0; k < H; ++k)
            logit += static_cast<double>(w2_axis[k]) * hidden[k];
        // Residual: W_skip[i] · x
        for (int j = 0; j < D; ++j)
            logit += static_cast<double>(wskip_axis[j]) * static_cast<double>(x[j]);

        // --- Calibration ---
        double prob;
        if (calibration_type_ == 1 && static_cast<size_t>(i) < platt_a_.size()) {
            // Platt scaling: sigmoid(a * logit + b)
            prob = sigmoid(static_cast<double>(platt_a_[i]) * logit + static_cast<double>(platt_b_[i]));
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
