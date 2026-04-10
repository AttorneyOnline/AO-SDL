#include "moderation/EmbeddingBackend.h"

#include "utils/Log.h"

#include <memory>
#include <string>

// The real llama.cpp backend is compiled in only when the build system
// defines KAGAMI_WITH_LLAMA_CPP. See CMakeLists.txt for the option and
// FetchContent pull. When it's off, make_embedding_backend() always
// returns a NullEmbeddingBackend, which lets the rest of the codebase
// be oblivious to whether ML support is present.
#ifdef KAGAMI_WITH_LLAMA_CPP
#include "llama.h"

#include <mutex>
#include <vector>

namespace moderation {

namespace {

/// Real backend backed by llama.cpp embedding mode. Loads a GGUF file
/// at construct time; the file must exist. Thread-safe: embed() takes
/// an internal mutex because llama_context mutations are not thread-
/// safe in llama.cpp.
class LlamaCppEmbeddingBackend : public EmbeddingBackend {
  public:
    explicit LlamaCppEmbeddingBackend(const std::string& model_path) {
        llama_backend_init();
        llama_model_params mparams = llama_model_default_params();
        mparams.use_mmap = true;
        model_ = llama_model_load_from_file(model_path.c_str(), mparams);
        if (!model_) {
            Log::log_print(ERR, "LlamaCpp: failed to load model %s", model_path.c_str());
            return;
        }
        llama_context_params cparams = llama_context_default_params();
        cparams.embeddings = true;
        cparams.n_batch = 512;
        cparams.n_ctx = 512;
        ctx_ = llama_init_from_model(model_, cparams);
        if (!ctx_) {
            Log::log_print(ERR, "LlamaCpp: failed to create context for %s", model_path.c_str());
            return;
        }
        dimension_ = llama_model_n_embd(model_);
        ready_ = true;
        Log::log_print(INFO, "LlamaCpp: loaded %s (dim=%d)", model_path.c_str(), dimension_);
    }

    ~LlamaCppEmbeddingBackend() override {
        if (ctx_)
            llama_free(ctx_);
        if (model_)
            llama_model_free(model_);
        llama_backend_free();
    }

    int dimension() const override {
        return dimension_;
    }
    bool is_ready() const override {
        return ready_;
    }
    const char* name() const override {
        return "llama.cpp";
    }

    EmbeddingResult embed(std::string_view text) override {
        EmbeddingResult out;
        if (!ready_) {
            out.error = "backend not ready";
            return out;
        }
        if (text.empty()) {
            out.error = "empty text";
            return out;
        }
        std::lock_guard lock(mu_);

        // Tokenize. llama_tokenize writes the token ids into a caller-
        // allocated buffer; we grow on overflow.
        std::vector<llama_token> tokens(text.size() + 16);
        auto vocab = llama_model_get_vocab(model_);
        int n = llama_tokenize(vocab, text.data(), static_cast<int>(text.size()),
                               tokens.data(), static_cast<int>(tokens.size()),
                               /*add_special=*/true, /*parse_special=*/false);
        if (n < 0) {
            tokens.resize(-n);
            n = llama_tokenize(vocab, text.data(), static_cast<int>(text.size()),
                               tokens.data(), static_cast<int>(tokens.size()),
                               true, false);
        }
        if (n <= 0) {
            out.error = "tokenize failed";
            return out;
        }
        tokens.resize(n);

        // Build a batch with all tokens in a single sequence.
        llama_batch batch = llama_batch_init(static_cast<int>(tokens.size()), 0, 1);
        for (int i = 0; i < n; ++i) {
            batch.token[i] = tokens[i];
            batch.pos[i] = i;
            batch.n_seq_id[i] = 1;
            batch.seq_id[i][0] = 0;
            batch.logits[i] = 0;
        }
        batch.logits[n - 1] = 1;
        batch.n_tokens = n;

        llama_memory_clear(llama_get_memory(ctx_), true);
        if (llama_decode(ctx_, batch) != 0) {
            llama_batch_free(batch);
            out.error = "decode failed";
            return out;
        }

        const float* emb = llama_get_embeddings_seq(ctx_, 0);
        if (!emb)
            emb = llama_get_embeddings(ctx_);
        if (!emb) {
            llama_batch_free(batch);
            out.error = "no embeddings";
            return out;
        }

        out.vector.assign(emb, emb + dimension_);
        // L2-normalize so cosine similarity is just a dot product.
        double norm_sq = 0.0;
        for (float v : out.vector)
            norm_sq += static_cast<double>(v) * static_cast<double>(v);
        if (norm_sq > 0.0) {
            const float inv = static_cast<float>(1.0 / std::sqrt(norm_sq));
            for (float& v : out.vector)
                v *= inv;
        }

        llama_batch_free(batch);
        out.ok = true;
        return out;
    }

  private:
    llama_model* model_ = nullptr;
    llama_context* ctx_ = nullptr;
    int dimension_ = 0;
    bool ready_ = false;
    std::mutex mu_;
};

} // namespace

std::unique_ptr<EmbeddingBackend> make_embedding_backend(const std::string& model_path) {
    if (model_path.empty())
        return std::make_unique<NullEmbeddingBackend>();
    auto backend = std::make_unique<LlamaCppEmbeddingBackend>(model_path);
    if (!backend->is_ready()) {
        Log::log_print(WARNING, "LlamaCpp: falling back to null backend");
        return std::make_unique<NullEmbeddingBackend>();
    }
    return backend;
}

} // namespace moderation

#else // !KAGAMI_WITH_LLAMA_CPP

namespace moderation {

std::unique_ptr<EmbeddingBackend> make_embedding_backend(const std::string& /*model_path*/) {
    // Built without llama.cpp: the embeddings layer is always the null
    // backend. The rest of the moderation stack keeps working.
    return std::make_unique<NullEmbeddingBackend>();
}

} // namespace moderation

#endif
