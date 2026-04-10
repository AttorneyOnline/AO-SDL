#include "moderation/UrlExtractor.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <string>

namespace moderation {

namespace {

/// Lowercase in place. We only lowercase ASCII because the regex only
/// matches ASCII characters in URLs, and IDN homoglyph attacks are the
/// UnicodeClassifier's problem — not ours.
std::string to_lower(std::string s) {
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

/// Case-insensitive substring search over ASCII haystack/needle.
bool contains_ci(std::string_view haystack, std::string_view needle) {
    if (needle.empty())
        return false;
    if (needle.size() > haystack.size())
        return false;
    auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    });
    return it != haystack.end();
}

/// Regex for http(s)://... URLs and bare domain-like tokens.
/// Constructed once on first use; std::regex is expensive to build
/// but cheap to match repeatedly.
///
/// Bare-domain rules:
///   - At least one dot
///   - TLD = 2-24 ASCII letters
///   - Preceding label = 2+ chars (prevents "a.js", "b.cc" false-
///     positives on common filenames; real domains almost always
///     have multi-char labels, and the few 1-char label cases are
///     rare enough that a false-negative is better than false-
///     positives on every `node.js`, `main.cpp`, `config.yml`
///     source-code reference in OOC chat)
///   - Explicit TLD allowlist of common 2-4 letter TLDs OR any
///     6+ letter TLD. This blocks the longtail of programming
///     language file extensions (.cpp, .hpp, .rs, .py, .js, .ts,
///     .go, .kt, etc.) that would otherwise match the generic
///     "2-24 ASCII letters" rule.
///
/// Note: this is still heuristic — a determined attacker can post
/// "example.com" without any URL markup and get picked up. The
/// goal is "catch casual spam/phishing URLs with few false
/// positives on source-code discussion", not "bulletproof URL
/// detection". The allowlist/blocklist config handles the
/// remaining policy.
const std::regex& url_regex() {
    static const std::regex re(
        R"((https?://[^\s<>"']+|)"
        R"((?:[a-zA-Z0-9][-a-zA-Z0-9]*\.)*[a-zA-Z0-9][-a-zA-Z0-9]{1,}\.)"
        R"((?:com|org|net|edu|gov|io|co|ru|ly|cc|me|tv|app|dev|xyz|info|biz|uk|us|de|fr|jp|cn|ca|au|nz|br|in|eu|[a-zA-Z]{6,})\b)"
        R"((?:/[^\s<>"']*)?))",
        std::regex::ECMAScript | std::regex::icase);
    return re;
}

} // namespace

UrlExtraction UrlExtractor::extract(std::string_view message) const {
    UrlExtraction out;
    if (!cfg_.enabled || message.empty())
        return out;

    // std::regex_iterator wants a std::string (no string_view overload).
    // The message is already bounded by max_{ic,ooc}_message_length so
    // the copy is small.
    std::string text(message);

    auto begin = std::sregex_iterator(text.begin(), text.end(), url_regex());
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::string url = to_lower(it->str());
        if (url.empty())
            continue;

        // Strip trailing punctuation that commonly follows a URL in prose
        // (",", ".", ")", "]", "!", "?").
        while (!url.empty()) {
            char c = url.back();
            if (c == ',' || c == '.' || c == ')' || c == ']' || c == '!' || c == '?' || c == ';')
                url.pop_back();
            else
                break;
        }
        if (url.empty())
            continue;

        // Allowlist takes precedence. If any allow substring matches,
        // this URL is explicitly safe and doesn't even count as "unknown".
        bool allowed = false;
        for (const auto& allow : cfg_.allowlist) {
            if (!allow.empty() && contains_ci(url, allow)) {
                allowed = true;
                break;
            }
        }
        if (allowed)
            continue;

        out.urls.push_back(url);

        bool blocked = false;
        for (const auto& needle : cfg_.blocklist) {
            if (!needle.empty() && contains_ci(url, needle)) {
                out.blocked.push_back(url);
                if (!out.reason.empty())
                    out.reason += ", ";
                out.reason += "blocked:" + needle;
                blocked = true;
                break;
            }
        }
        (void)blocked;
    }

    if (!out.blocked.empty()) {
        out.score = std::clamp(cfg_.blocked_score, 0.0, 1.0);
    }
    else if (!out.urls.empty() && cfg_.unknown_url_score > 0.0) {
        out.score = std::clamp(cfg_.unknown_url_score, 0.0, 1.0);
        if (out.reason.empty())
            out.reason = "unknown_url";
    }
    return out;
}

} // namespace moderation
