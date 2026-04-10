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
    auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
                          [](char a, char b) {
                              return std::tolower(static_cast<unsigned char>(a)) ==
                                     std::tolower(static_cast<unsigned char>(b));
                          });
    return it != haystack.end();
}

/// Regex for http(s)://... URLs and bare domain-like tokens.
/// Constructed once on first use; std::regex is expensive to build
/// but cheap to match repeatedly.
const std::regex& url_regex() {
    // Matches:
    //   - http(s)://anything-up-to-whitespace
    //   - bare domains of the form foo.bar(.baz)? followed by optional path
    // The bare-domain regex deliberately requires at least one dot and
    // a TLD of 2-24 ASCII letters, so we don't match things like "hi.u".
    static const std::regex re(
        R"((https?://[^\s<>"']+|(?:[a-zA-Z0-9][-a-zA-Z0-9]*\.)+[a-zA-Z]{2,24}(?:/[^\s<>"']*)?))",
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
