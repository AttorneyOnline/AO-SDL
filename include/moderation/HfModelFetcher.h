/**
 * @file HfModelFetcher.h
 * @brief Resolve a HuggingFace model id to a local GGUF file on disk.
 *
 * The embeddings layer expects a GGUF file that llama.cpp can load.
 * Operators configure an HF repo id in kagami.json; this helper is
 * responsible for turning that id into a local path, downloading the
 * file if it's not already cached.
 *
 * Behavior:
 *   1. If the id is already a filesystem path and the file exists,
 *      return it unchanged.
 *   2. Otherwise, assume the id is a HuggingFace repo (e.g.
 *      "ChristianAzinn/all-MiniLM-L6-v2-gguf"). The fetcher builds a
 *      URL of the form
 *      https://huggingface.co/{repo}/resolve/main/{filename} and
 *      downloads it to @p cache_dir/{sanitized_id}.gguf.
 *   3. If the id contains a ":filename" suffix, that filename is used
 *      instead of a default guess.
 *
 * The fetcher is intentionally synchronous and run only at startup.
 * A background fetch would complicate shutdown semantics for no real
 * benefit — the server simply waits for the download on first boot.
 */
#pragma once

#include <string>

namespace moderation {

struct HfFetchResult {
    bool ok = false;
    std::string local_path;  ///< Filesystem path to the loaded .gguf file.
    std::string error;
    bool downloaded = false; ///< True if a network fetch happened; false for cache hits.
};

/// Resolve a HF model id to a local path.
///
/// @param hf_id      HuggingFace identifier, e.g. "owner/repo" or
///                   "owner/repo:specific-file.gguf". May also be an
///                   absolute path on disk.
/// @param cache_dir  Directory where downloaded files are cached.
///                   Will be created if it doesn't exist.
HfFetchResult resolve_hf_model(const std::string& hf_id, const std::string& cache_dir);

} // namespace moderation
