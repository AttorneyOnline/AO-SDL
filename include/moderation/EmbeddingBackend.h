/**
 * @file EmbeddingBackend.h
 * @brief Pluggable embedding backend interface for the semantic layer.
 *
 * The semantic spam clustering logic (ring buffer + cosine similarity
 * + cross-IPID count) lives in SemanticClusterer and is independent
 * of any particular embedding model. EmbeddingBackend is the trait
 * that produces the vector for a given text.
 *
 * Two implementations are provided:
 *
 *   - NullEmbeddingBackend — always returns `not_ready`. Default.
 *     Keeps the codebase buildable without ML dependencies and lets
 *     tests exercise the clustering logic in isolation.
 *
 *   - LlamaCppEmbeddingBackend — wraps llama.cpp in embedding mode.
 *     Compiled only when KAGAMI_WITH_LLAMA_CPP is defined. Loads a
 *     GGUF file from disk at construct time; the file must exist.
 *     The HF fetch is done by HfModelFetcher before construction.
 *
 * Thread safety: embed() may be called concurrently from multiple
 * threads. Implementations are expected to serialize internally if
 * the underlying library isn't thread-safe.
 */
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace moderation {

struct EmbeddingResult {
    bool ok = false;
    std::vector<float> vector; ///< Unit-normalized embedding vector (L2).
    std::string error;
    int token_count = 0;     ///< # tokens after tokenization, 0 on failure.
    int64_t tokenize_ns = 0; ///< Nanoseconds spent in the tokenizer.
    int64_t decode_ns = 0;   ///< Nanoseconds spent in llama_decode (model forward pass).
};

class EmbeddingBackend {
  public:
    virtual ~EmbeddingBackend() = default;

    /// Dimensionality of the embedding vector. Must be constant over
    /// the lifetime of the backend. 0 before the model has loaded.
    virtual int dimension() const = 0;

    /// True if the backend is ready to serve embed() calls.
    virtual bool is_ready() const = 0;

    /// Produce an embedding for @p text. Empty text returns ok=false.
    virtual EmbeddingResult embed(std::string_view text) = 0;

    /// Backend name for metrics and logs.
    virtual const char* name() const = 0;
};

/// A backend that never produces embeddings. Used when the layer is
/// disabled or no model is configured. embed() always returns ok=false
/// with a "not_ready" error.
class NullEmbeddingBackend : public EmbeddingBackend {
  public:
    int dimension() const override {
        return 0;
    }
    bool is_ready() const override {
        return false;
    }
    EmbeddingResult embed(std::string_view /*text*/) override {
        EmbeddingResult r;
        r.error = "not_ready";
        return r;
    }
    const char* name() const override {
        return "null";
    }
};

/// Factory for the default backend. Returns NullEmbeddingBackend when
/// llama.cpp is not compiled in, or a LlamaCppEmbeddingBackend wrapping
/// the given model file when it is.
std::unique_ptr<EmbeddingBackend> make_embedding_backend(const std::string& model_path);

} // namespace moderation
