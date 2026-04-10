/**
 * @file UnicodeClassifier.h
 * @brief Rule-based visual-noise scoring for chat messages.
 *
 * This classifier answers exactly one question: "how visually loud is
 * this message?" It does NOT attempt semantic classification — that
 * is the remote classifier's job. This layer catches:
 *
 *   - Zalgo text: excessive combining marks stacked on base characters.
 *   - Cuneiform / Tags / private-use blasts: long runs of rare scripts.
 *   - Invisible-character attacks: RLO/LRO/ZWJ abuse.
 *   - Script mixing: "аррlе" (Cyrillic 'а' masquerading as Latin 'a').
 *
 * The scorer is intentionally dumb and deterministic. Given the same
 * message and the same config, it returns the same score every time,
 * which makes it easy to regression-test.
 */
#pragma once

#include "moderation/ContentModerationConfig.h"

#include <string>
#include <string_view>

namespace moderation {

/// Result of classifying a single message.
struct UnicodeClassification {
    double score = 0.0;       ///< Final score in [0, config.max_score].
    int total_codepoints = 0; ///< Parsed codepoint count (not bytes).
    int combining_marks = 0;  ///< Count of Mn/Mc/Me category codepoints.
    int format_chars = 0;     ///< Count of Cf (format) codepoints.
    int private_use = 0;      ///< Count of Co (private-use area) codepoints.
    int exotic_script = 0;    ///< Count of rare-script codepoints.
    int script_count = 0;     ///< Number of distinct major scripts seen.
    std::string reason;       ///< Human-readable summary ("zalgo", "cuneiform", etc.).
};

class UnicodeClassifier {
  public:
    UnicodeClassifier() = default;

    void configure(const UnicodeLayerConfig& cfg) {
        cfg_ = cfg;
    }

    /// Classify a UTF-8-encoded message. Invalid UTF-8 counts toward
    /// the score directly (garbage in = suspicious out).
    UnicodeClassification classify(std::string_view message) const;

    /// Convenience: just the score. Returns 0.0 if the layer is disabled.
    double score(std::string_view message) const {
        if (!cfg_.enabled)
            return 0.0;
        return classify(message).score;
    }

  private:
    UnicodeLayerConfig cfg_;
};

} // namespace moderation
