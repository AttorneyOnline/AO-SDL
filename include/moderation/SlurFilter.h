/**
 * @file SlurFilter.h
 * @brief Layer 1c: word-boundary slur wordlist check with
 *        Scunthorpe-safe matching.
 *
 * Purpose
 * -------
 * OpenAI's moderation API has a categorical recall problem on common
 * casual slurs: common pro-Nazi phrases score hate ~= 0.00014, common
 * trans-targeting slurs ~= 0.05, various racial epithets in the
 * 0.01-0.3 range — the hate axis is narrowly tuned to overt identity-
 * targeted hostility. No floor tweak can rescue this — a 0.01 floor
 * would catch everything including normal dialogue. The mitigation is
 * an explicit wordlist checked before we spend money on a remote call.
 *
 * Design constraints
 * ------------------
 *  1. No wordlist in the source tree. Operators supply a URL and
 *     TextListFetcher pulls it at startup. The repo stays clean.
 *  2. Word-boundary matching only. A substring check would fire on
 *     `scunthorpe`, `therapist`, `classify`, `assassin`, `bangalore`,
 *     `penistone`, `analysis`, etc. Every one of those is a real
 *     Scunthorpe-problem case from production filter failures and
 *     we are not repeating them.
 *  3. Homoglyph + leet fold. Attackers substitute `1` for `i` (leet),
 *     Cyrillic `\u0435` for `e`, zero-width joins like `\u200b`
 *     inserted mid-word, or Cyrillic `і` for Latin `i`. The
 *     normalizer folds ASCII-mappable confusables back to their
 *     ASCII form BEFORE word-boundary tokenization.
 *  4. Exception list. A companion list of words that the normalizer
 *     and matcher would otherwise fire on, supplied the same way.
 *     If the exception list contains a token that appears in the
 *     message, matches on that token are suppressed. This is a
 *     belt-and-suspenders against normalizer bugs or community
 *     words that collide with the wordlist (e.g. reclaimed slurs
 *     used by in-group members — operators can exempt them).
 *  5. Limited suffix allowance. `-s`, `-es`, `-ed`, `-ing`, `-er`,
 *     `-ers`. Keeps `scores` from matching a `score` in the list
 *     without needing every inflection in the list.
 *  6. Stateless extract — a separate SlurFilter instance holds the
 *     wordlist and can be swapped in at runtime when the background
 *     fetch completes, identical pattern to the embedding backend.
 *
 * Scope clarification
 * -------------------
 * This layer targets extremist hate speech, not profanity. Crude
 * language and insults are the remote classifier's problem (and even
 * then, tuned high for roleplay). The operator-supplied wordlist is
 * meant to be curated: ~200-500 entries covering identity-targeted
 * slurs, ethnic epithets, and extremist terminology. A 10,000-entry
 * "banned words" list is NOT the intended use case — that would
 * cause every message in a Discord-adjacent chat to get censored.
 */
#pragma once

#include "moderation/ContentModerationConfig.h"

#include <mutex>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace moderation {

struct SlurMatchResult {
    double score = 0.0;               ///< 0 if no match, match_score if any token hit.
    std::vector<std::string> matched; ///< Normalized tokens that fired. For audit log only.
    std::string reason;               ///< Human-readable summary. "slur(3 matches)" etc.
};

class SlurFilter {
  public:
    SlurFilter() = default;

    /// Configure the scalar policy (enabled, match_score). This does
    /// NOT load the wordlist — the caller is expected to call
    /// load_wordlist() / load_exceptions() from a background thread
    /// once the TextListFetcher completes.
    void configure(const SlurLayerConfig& cfg);

    /// Install the wordlist. Normalized once at load time so the hot
    /// path only normalizes the message, not both sides.
    /// Safe to call at runtime: holds mu_ during the swap.
    void load_wordlist(const std::vector<std::string>& raw);

    /// Install the exception list. Same semantics as load_wordlist.
    void load_exceptions(const std::vector<std::string>& raw);

    /// Number of loaded wordlist entries (post-normalization, dedup).
    size_t wordlist_size() const;

    /// Number of loaded exception entries.
    size_t exception_size() const;

    /// True if the layer has any terms loaded. Used by ContentModerator
    /// to skip the layer entirely before it even takes mu_ — no point
    /// normalizing every message when the list is empty.
    bool is_active() const;

    /// Scan a message for wordlist matches. Returns score=0 if none,
    /// score=match_score otherwise. Thread-safe: uses a shared lock
    /// so concurrent scans don't serialize.
    SlurMatchResult scan(std::string_view message) const;

    // --- internals exposed for tests ---------------------------------

    /// Normalize a UTF-8 string: NFKC-ish folding (lowercase, strip
    /// combining marks, fold common homoglyphs to ASCII, strip
    /// zero-width characters, collapse repeated letters, digit-to-
    /// letter leet fold). Exposed so unit tests can assert on the
    /// normalizer output directly.
    static std::string normalize(std::string_view message);

    /// Tokenize a normalized string on non-alphanumeric boundaries.
    /// A "word" is a maximal run of ASCII [a-z0-9].
    static std::vector<std::string> tokenize(std::string_view normalized);

    /// Strip a limited set of common suffixes (-s, -es, -ed, -ing,
    /// -er, -ers). Returns the stem if stripping succeeded, or the
    /// input unchanged. Used by scan() to match both the bare form
    /// and common inflections.
    static std::string_view strip_suffix(std::string_view token);

  private:
    SlurLayerConfig cfg_{};

    // Storage is a pair of hash sets of normalized tokens.
    // load_wordlist/load_exceptions re-normalize the raw list and
    // populate these under mu_. scan() takes mu_ shared.
    mutable std::mutex mu_;
    std::unordered_set<std::string> wordlist_;
    std::unordered_set<std::string> exceptions_;
};

} // namespace moderation
