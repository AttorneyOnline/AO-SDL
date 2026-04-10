/**
 * @file UrlExtractor.h
 * @brief Lightweight URL/domain extraction with offline blocklist matching.
 *
 * Phase 1 is deliberately offline: no Safe Browsing lookups, no async
 * reputation queries, no DNS. All we do is:
 *
 *   1. Pull out URL-ish tokens via a simple regex.
 *   2. Normalize (lowercase host).
 *   3. Match each token against an operator-supplied blocklist of
 *      substrings. An allowlist overrides.
 *
 * That covers the "paste a phishing link" case for zero cost. A future
 * phase can feed extracted URLs into an async reputation checker.
 */
#pragma once

#include "moderation/ContentModerationConfig.h"

#include <string>
#include <string_view>
#include <vector>

namespace moderation {

struct UrlExtraction {
    std::vector<std::string> urls;    ///< Normalized URLs found in the message.
    std::vector<std::string> blocked; ///< Subset of `urls` that matched the blocklist.
    double score = 0.0;               ///< Aggregate risk score in [0, 1].
    std::string reason;               ///< Which blocklist entries hit.
};

class UrlExtractor {
  public:
    UrlExtractor() = default;

    void configure(const UrlLayerConfig& cfg) {
        cfg_ = cfg;
    }

    /// Extract URLs and score them. Returns an empty result with
    /// score=0 if the layer is disabled.
    UrlExtraction extract(std::string_view message) const;

  private:
    UrlLayerConfig cfg_;
};

} // namespace moderation
