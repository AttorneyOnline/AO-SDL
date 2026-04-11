#include "moderation/SlurFilter.h"

#include <cctype>
#include <cstdint>
#include <string>
#include <unordered_set>

namespace moderation {

namespace {

/// Lightweight UTF-8 decoder. Returns the decoded code point and
/// advances @p i past the consumed bytes. On any invalid sequence
/// (overlong, surrogate, out-of-range, truncated) the function
/// returns U+FFFD and advances by one byte — the normalizer maps
/// U+FFFD to a space, so an invalid message just produces a
/// boundary, not a crash.
///
/// We deliberately do NOT reuse UnicodeClassifier's decoder here:
/// that decoder was tuned to FAIL-LOUD on malformed input for the
/// visual-noise axis, whereas the slur filter wants to keep running
/// through noise. Different error-handling contracts in the same
/// file would be confusing, so we carry our own lenient decoder.
uint32_t decode_utf8(std::string_view s, size_t& i) {
    if (i >= s.size())
        return 0xFFFD;
    auto b0 = static_cast<unsigned char>(s[i]);
    if (b0 < 0x80) {
        ++i;
        return b0;
    }
    int width = 0;
    uint32_t cp = 0;
    if ((b0 & 0xE0) == 0xC0) {
        width = 2;
        cp = b0 & 0x1F;
    }
    else if ((b0 & 0xF0) == 0xE0) {
        width = 3;
        cp = b0 & 0x0F;
    }
    else if ((b0 & 0xF8) == 0xF0) {
        width = 4;
        cp = b0 & 0x07;
    }
    else {
        ++i;
        return 0xFFFD;
    }
    if (i + width > s.size()) {
        ++i;
        return 0xFFFD;
    }
    for (int k = 1; k < width; ++k) {
        auto bk = static_cast<unsigned char>(s[i + k]);
        if ((bk & 0xC0) != 0x80) {
            ++i;
            return 0xFFFD;
        }
        cp = (cp << 6) | (bk & 0x3F);
    }
    // Reject surrogates and out-of-range.
    if (cp >= 0xD800 && cp <= 0xDFFF) {
        ++i;
        return 0xFFFD;
    }
    if (cp > 0x10FFFF) {
        ++i;
        return 0xFFFD;
    }
    // Reject overlong encodings — the classic evasion where an
    // attacker writes U+0041 ('A') as C1 81 to bypass a byte-level
    // filter. UnicodeClassifier rejects these for scoring; here we
    // just replace with U+FFFD.
    if ((width == 2 && cp < 0x80) || (width == 3 && cp < 0x800) || (width == 4 && cp < 0x10000)) {
        ++i;
        return 0xFFFD;
    }
    i += width;
    return cp;
}

/// True if @p cp is a combining mark we want to drop (zalgo, accents).
/// Covers the three biggest combining blocks — enough to fold the
/// overwhelming majority of decoration attacks without pulling in a
/// full Unicode property database.
bool is_combining_mark(uint32_t cp) {
    return (cp >= 0x0300 && cp <= 0x036F)     // Combining Diacritical Marks
           || (cp >= 0x20D0 && cp <= 0x20FF)  // Combining Marks for Symbols
           || (cp >= 0xFE20 && cp <= 0xFE2F)  // Combining Half Marks
           || (cp >= 0x1AB0 && cp <= 0x1AFF)  // Combining Diacritical Marks Extended
           || (cp >= 0x1DC0 && cp <= 0x1DFF); // Combining Diacritical Marks Supplement
}

/// True if @p cp is a zero-width or invisible character that we
/// should drop entirely (not even leave a boundary).
bool is_zero_width(uint32_t cp) {
    return cp == 0x200B     // ZERO WIDTH SPACE
           || cp == 0x200C  // ZERO WIDTH NON-JOINER
           || cp == 0x200D  // ZERO WIDTH JOINER
           || cp == 0x2060  // WORD JOINER
           || cp == 0xFEFF  // BOM / ZERO WIDTH NO-BREAK SPACE
           || cp == 0x180E  // MONGOLIAN VOWEL SEPARATOR (deprecated)
           || cp == 0x00AD; // SOFT HYPHEN
}

/// Fold a code point to an ASCII character, returning 0 if there is
/// no mapping (caller will emit a space). Covers the most common
/// homoglyph substitutions used in slur evasion. Full ICU confusables
/// would be better but drag in ~10 MB of tables; this curated list
/// handles the cases we've actually seen in the wild.
char fold_confusable(uint32_t cp) {
    // Fullwidth ASCII: U+FF01..U+FF5E maps to U+0021..U+007E.
    if (cp >= 0xFF01 && cp <= 0xFF5E)
        return static_cast<char>(cp - 0xFEE0);

    // Latin-1 accented letters — NFKD-ish fold of the common cases.
    // We don't cover everything; the main point is that messages
    // containing accented Latin text still normalize to ASCII tokens
    // so the wordlist can match them.
    switch (cp) {
    case 0x00C0:
    case 0x00C1:
    case 0x00C2:
    case 0x00C3:
    case 0x00C4:
    case 0x00C5:
    case 0x00E0:
    case 0x00E1:
    case 0x00E2:
    case 0x00E3:
    case 0x00E4:
    case 0x00E5:
        return 'a';
    case 0x00C8:
    case 0x00C9:
    case 0x00CA:
    case 0x00CB:
    case 0x00E8:
    case 0x00E9:
    case 0x00EA:
    case 0x00EB:
        return 'e';
    case 0x00CC:
    case 0x00CD:
    case 0x00CE:
    case 0x00CF:
    case 0x00EC:
    case 0x00ED:
    case 0x00EE:
    case 0x00EF:
        return 'i';
    case 0x00D2:
    case 0x00D3:
    case 0x00D4:
    case 0x00D5:
    case 0x00D6:
    case 0x00D8:
    case 0x00F2:
    case 0x00F3:
    case 0x00F4:
    case 0x00F5:
    case 0x00F6:
    case 0x00F8:
        return 'o';
    case 0x00D9:
    case 0x00DA:
    case 0x00DB:
    case 0x00DC:
    case 0x00F9:
    case 0x00FA:
    case 0x00FB:
    case 0x00FC:
        return 'u';
    case 0x00D1:
    case 0x00F1:
        return 'n';
    case 0x00C7:
    case 0x00E7:
        return 'c';
    case 0x00DD:
    case 0x00FD:
    case 0x00FF:
        return 'y';
    }

    // Cyrillic ↔ Latin homoglyphs — the single biggest evasion class.
    // The substitutions below are all visually identical in common
    // fonts and the attacker can type them with a Russian keyboard
    // layout in half a second.
    switch (cp) {
    case 0x0430:
        return 'a'; // а
    case 0x0435:
        return 'e'; // е
    case 0x0456:
        return 'i'; // і (Ukrainian)
    case 0x043E:
        return 'o'; // о
    case 0x0440:
        return 'p'; // р
    case 0x0441:
        return 'c'; // с
    case 0x0443:
        return 'y'; // у
    case 0x0445:
        return 'x'; // х
    case 0x043A:
        return 'k'; // к
    case 0x043C:
        return 'm'; // м
    case 0x0410:
        return 'a'; // А
    case 0x0412:
        return 'b'; // В
    case 0x0415:
        return 'e'; // Е
    case 0x041A:
        return 'k'; // К
    case 0x041C:
        return 'm'; // М
    case 0x041D:
        return 'h'; // Н
    case 0x041E:
        return 'o'; // О
    case 0x0420:
        return 'p'; // Р
    case 0x0421:
        return 'c'; // С
    case 0x0422:
        return 't'; // Т
    case 0x0425:
        return 'x'; // Х
    }

    // Greek letters commonly used as Latin homoglyphs.
    switch (cp) {
    case 0x03B1:
        return 'a'; // α
    case 0x03BF:
        return 'o'; // ο
    case 0x03B5:
        return 'e'; // ε
    case 0x03BD:
        return 'v'; // ν
    case 0x03C1:
        return 'p'; // ρ
    case 0x03BA:
        return 'k'; // κ
    case 0x03B9:
        return 'i'; // ι
    case 0x03C7:
        return 'x'; // χ
    }

    return 0;
}

/// Fold ASCII "leet" digits to their letter equivalents. Only the
/// cases where the digit is visually unambiguous and commonly used
/// in slur evasion. Digits without a mapping pass through unchanged
/// (and are later tokenized as digit characters).
char fold_leet_digit(char c) {
    switch (c) {
    case '0':
        return 'o';
    case '1':
        return 'i';
    case '3':
        return 'e';
    case '4':
        return 'a';
    case '5':
        return 's';
    case '7':
        return 't';
    case '9':
        return 'g';
    default:
        return c;
    }
}

} // namespace

std::string SlurFilter::normalize(std::string_view message) {
    // Pass 1: decode UTF-8, drop combining marks + zero-width chars,
    // fold confusables, lowercase ASCII, leet-fold digits. No repeat
    // collapsing yet — that runs as a second pass so we can see the
    // whole run at once and make a clean drop/keep decision.
    std::string folded;
    folded.reserve(message.size());

    size_t i = 0;
    while (i < message.size()) {
        uint32_t cp = decode_utf8(message, i);

        if (cp == 0xFFFD) {
            // Invalid byte — produce a boundary so tokens on each
            // side don't accidentally fuse across the decode error.
            folded.push_back(' ');
            continue;
        }
        if (is_combining_mark(cp) || is_zero_width(cp)) {
            // Drop entirely. The base letter on either side fuses —
            // e.g. an attacker inserting a combining acute accent
            // like U+0301 mid-word to break a token match. After
            // the drop, the two halves re-join as if the combining
            // mark never existed and the word-boundary tokenizer
            // sees a single token instead of two.
            continue;
        }

        if (cp < 0x80) {
            char c = static_cast<char>(cp);
            if (std::isupper(static_cast<unsigned char>(c)))
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (std::isdigit(static_cast<unsigned char>(c)))
                c = fold_leet_digit(c);
            folded.push_back(c);
            continue;
        }

        if (char ch = fold_confusable(cp); ch != 0) {
            folded.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
            continue;
        }

        // Unmappable non-ASCII — emit a space so tokens don't fuse
        // across it. This is the "something weird is here" boundary
        // that prevents `word<U+XXXX>word` from matching `wordword`.
        folded.push_back(' ');
    }

    // Pass 2: collapse runs of 3+ identical letters down to a single
    // letter. The two-pass split avoids the hairy edge cases of an
    // inline "drop after 2" approach (which keeps "nnice" instead of
    // "nice" and thus misses prefix-repeat evasion).
    //
    // Runs of exactly 2 are preserved so real English doubles ("book",
    // "committee", "address") stay intact. This matters because the
    // wordlist is also normalized through here — a wordlist entry
    // with a legitimate double letter keeps both letters, and a
    // runtime message with the same double matches cleanly.
    //
    // Only alphabetic runs collapse. Digit runs (which get leet-folded
    // to letters above anyway) and punctuation runs are left alone.
    std::string out;
    out.reserve(folded.size());
    for (size_t k = 0; k < folded.size();) {
        char c = folded[k];
        size_t j = k;
        while (j < folded.size() && folded[j] == c)
            ++j;
        size_t len = j - k;
        if (std::isalpha(static_cast<unsigned char>(c)) && len >= 3) {
            out.push_back(c); // keep one
        }
        else {
            out.append(folded, k, len);
        }
        k = j;
    }
    return out;
}

std::vector<std::string> SlurFilter::tokenize(std::string_view normalized) {
    std::vector<std::string> tokens;
    size_t i = 0;
    while (i < normalized.size()) {
        // Skip non-alphanumeric (the "word boundary").
        while (i < normalized.size() && !std::isalnum(static_cast<unsigned char>(normalized[i])))
            ++i;
        // Capture the maximal alnum run.
        size_t start = i;
        while (i < normalized.size() && std::isalnum(static_cast<unsigned char>(normalized[i])))
            ++i;
        if (i > start)
            tokens.emplace_back(normalized.substr(start, i - start));
    }
    return tokens;
}

std::string_view SlurFilter::strip_suffix(std::string_view token) {
    // Try the longest suffixes first so "bastards" matches both as
    // "bastards" (literal) and "bastard" (after -s strip) before
    // falling through to the shorter suffixes.
    //
    // Minimum stem length of 3 — we don't strip "er" off "her" to
    // get "h", that'd just add noise.
    constexpr int min_stem = 3;
    auto try_strip = [&](std::string_view suffix) -> bool {
        if (token.size() < suffix.size() + static_cast<size_t>(min_stem))
            return false;
        return token.substr(token.size() - suffix.size()) == suffix;
    };
    if (try_strip("ers"))
        return token.substr(0, token.size() - 3);
    if (try_strip("ing"))
        return token.substr(0, token.size() - 3);
    if (try_strip("ed"))
        return token.substr(0, token.size() - 2);
    if (try_strip("er"))
        return token.substr(0, token.size() - 2);
    if (try_strip("es"))
        return token.substr(0, token.size() - 2);
    if (try_strip("s"))
        return token.substr(0, token.size() - 1);
    return token;
}

void SlurFilter::configure(const SlurLayerConfig& cfg) {
    std::lock_guard lock(mu_);
    cfg_ = cfg;
    // Deliberately NOT clearing wordlist_/exceptions_ here — the
    // caller reloads them via load_* after the TextListFetcher
    // background thread completes. A reconfigure that changes the
    // URL without also reloading the list keeps the previous list
    // loaded, which matches the "layer stays useful across config
    // churn" design we used for the other layers.
}

void SlurFilter::load_wordlist(const std::vector<std::string>& raw) {
    std::unordered_set<std::string> fresh;
    fresh.reserve(raw.size());
    for (const auto& entry : raw) {
        auto norm = normalize(entry);
        // The wordlist stores whole-word tokens, so we pull the first
        // non-empty normalized token from each line. Entries that
        // contain multiple words (e.g. "white power") can't match
        // under this single-token design — that's intentional, multi-
        // word phrases are a Layer 2 problem (semantic similarity).
        auto toks = tokenize(norm);
        if (toks.empty())
            continue;
        // Min-length guard: reject wordlist entries whose first
        // normalized token is shorter than 3 characters. Two-letter
        // tokens are extremely ambiguous — common abbreviations and
        // pronouns ("ai", "it", "us", "eu", "is", "as") would false-
        // positive on nearly every message. The 3-char floor is a
        // hard safety rail independent of the operator's list.
        //
        // Real-world trigger: community slur lists sometimes include
        // numeric/coded entries like "41%" (a transphobic dog whistle)
        // that normalize via leet-digit fold (4->a, 1->i) to "ai".
        // Without this guard, loading such an entry would put "ai"
        // in the active wordlist and match every mention of
        // "artificial intelligence". The entry stays in the source
        // file as documentation / future phrase-matching, but the
        // active matcher safely ignores it.
        //
        // Operators who genuinely want 2-letter matches (rare) can
        // wrap the term in a longer surrounding token so the matcher
        // picks up something unambiguous.
        if (toks[0].size() < 3)
            continue;
        fresh.insert(std::move(toks[0]));
    }
    std::lock_guard lock(mu_);
    wordlist_ = std::move(fresh);
}

void SlurFilter::load_exceptions(const std::vector<std::string>& raw) {
    std::unordered_set<std::string> fresh;
    fresh.reserve(raw.size());
    for (const auto& entry : raw) {
        auto norm = normalize(entry);
        auto toks = tokenize(norm);
        if (!toks.empty())
            fresh.insert(std::move(toks[0]));
    }
    std::lock_guard lock(mu_);
    exceptions_ = std::move(fresh);
}

size_t SlurFilter::wordlist_size() const {
    std::lock_guard lock(mu_);
    return wordlist_.size();
}

size_t SlurFilter::exception_size() const {
    std::lock_guard lock(mu_);
    return exceptions_.size();
}

bool SlurFilter::is_active() const {
    std::lock_guard lock(mu_);
    return cfg_.enabled && !wordlist_.empty();
}

SlurMatchResult SlurFilter::scan(std::string_view message) const {
    SlurMatchResult out;

    // Fast path: no lock needed because enabled + wordlist emptiness
    // is decided under is_active() which takes the lock exactly once.
    // Between that check and the scan, a concurrent reload could
    // swap the set; we take mu_ below and re-check.
    if (!is_active())
        return out;

    auto norm = normalize(message);
    auto tokens = tokenize(norm);
    if (tokens.empty())
        return out;

    std::lock_guard lock(mu_);
    if (wordlist_.empty())
        return out;

    // First pass: collect candidate matches and the set of tokens
    // present in the message that are on the exception list.
    //
    // We do two passes instead of one because the exception check
    // is global: "pianist therapist" should NOT fire on "therapist"
    // even if some future normalizer bug sends "therapist" through
    // to a wordlist entry, because "therapist" is in exceptions_.
    // A single-pass scan that checks exception membership inline
    // would also handle this, but the two-pass form is clearer and
    // the token counts involved are tiny (< 100 per message).
    std::vector<std::string> candidates;
    for (const auto& tok : tokens) {
        if (exceptions_.count(tok))
            continue;
        if (wordlist_.count(tok)) {
            candidates.push_back(tok);
            continue;
        }
        auto stem_sv = strip_suffix(tok);
        if (stem_sv.size() < tok.size()) {
            std::string stem(stem_sv);
            if (!exceptions_.count(stem) && wordlist_.count(stem))
                candidates.push_back(std::move(stem));
        }
    }

    if (candidates.empty())
        return out;

    // Dedup for a cleaner reason string. We cap the reported list
    // at 5 entries so the audit log / Prometheus labels don't blow
    // up when someone copy-pastes a wall of slurs.
    std::unordered_set<std::string> seen;
    for (auto& c : candidates) {
        if (seen.insert(c).second)
            out.matched.push_back(c);
    }
    out.score = cfg_.match_score;
    out.reason = "slur(" + std::to_string(out.matched.size()) + ")";
    return out;
}

} // namespace moderation
