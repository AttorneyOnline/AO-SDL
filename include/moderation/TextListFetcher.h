/**
 * @file TextListFetcher.h
 * @brief Fetch a small newline-delimited text list from an https URL,
 *        with a bounded byte cap and a simple filesystem cache.
 *
 * Used by Layer 1c (slur wordlist) and the safe-hint embedding layer
 * to load operator-supplied term lists from S3 without baking any
 * defaults into the repo. The "don't embed this in source" requirement
 * comes from the slur wordlist in particular: the repo shouldn't
 * contain the words, but a public S3 object is fine because OpenAI
 * moderation is expected to be a second line of defense.
 *
 * Design notes (see HfModelFetcher for the parallel download path):
 *   - https-only, including redirects — a 3xx pointing at http:// is
 *     refused, not silently downgraded. Parity with HfModelFetcher.
 *   - Hard byte cap (1 MiB). Slur wordlists are in the thousands of
 *     entries and weigh <50 KB; 1 MiB leaves plenty of headroom without
 *     letting a misconfigured URL OOM the server on a 2 GB model file.
 *   - Cache to disk. On failure (network, http, parse), if a stale
 *     cache file exists we return its contents so a transient S3
 *     outage doesn't leave Layer 1c inert. Fresh boots with no cache
 *     fall through to the empty list (layer disables itself).
 *   - Synchronous. The caller is expected to dispatch this on a
 *     background thread at startup — same pattern main.cpp uses for
 *     HfModelFetcher::resolve_hf_model.
 *   - Parsing is intentionally generic: strips BOM, trims whitespace,
 *     drops blank lines and `#`-prefixed comments. The layer that
 *     uses the list (SlurFilter, SafeHintLayer) does its own
 *     normalization on top.
 */
#pragma once

#include <string>
#include <vector>

namespace moderation {

struct TextListFetchResult {
    bool ok = false;
    std::vector<std::string> entries; ///< One line per entry, trimmed, no comments/blanks.
    std::string local_path;           ///< Cache file path, set whether we fetched or loaded from cache.
    std::string error;                ///< Populated when ok == false.
    bool from_cache = false;          ///< True if we fell back to the cache file (network failed).
    bool downloaded = false;          ///< True if we completed a fresh download.
};

/// Fetch a newline-delimited text list.
///
/// @param url        https:// URL to fetch. Empty URL returns ok=false
///                   with a descriptive error (callers treat the empty-
///                   URL case as "layer disabled" upstream).
/// @param cache_dir  Directory for the cache file. Created if missing.
/// @param cache_name Sanitized filename within @p cache_dir. The caller
///                   chooses this (e.g. "slurs.txt", "safe_anchors.txt")
///                   so operators can inspect the cached copy.
///
/// Behavior:
///   1. HEAD-style fetch with streaming receiver. Hard 1 MiB cap.
///   2. Follow up to 5 https-only redirects.
///   3. On success: parse into `entries`, write the raw body to
///      cache_dir/cache_name, return ok=true.
///   4. On failure: if cache file exists and is readable, load it,
///      return ok=true with from_cache=true. Otherwise return ok=false.
TextListFetchResult fetch_text_list(const std::string& url, const std::string& cache_dir,
                                    const std::string& cache_name);

/// Parse a raw body into a list of trimmed entries.
/// Exposed for unit tests and for callers that already have the bytes
/// on hand from somewhere other than this fetcher.
std::vector<std::string> parse_text_list(std::string_view body);

} // namespace moderation
