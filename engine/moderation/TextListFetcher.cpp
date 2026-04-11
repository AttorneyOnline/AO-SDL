#include "moderation/TextListFetcher.h"

#include "net/Http.h"
#include "utils/Log.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace moderation {

namespace {

/// Text lists are small: slur wordlists we've seen in the wild are
/// ~5-50 KB, safe-hint anchors are ~1 KB. 1 MiB is 20x-1000x the
/// expected size, large enough for any reasonable community list
/// without letting a misconfigured URL OOM the host.
constexpr size_t kMaxListBytes = 1ULL * 1024 * 1024;

/// Maximum redirect hops. Same as HfModelFetcher.
constexpr int kMaxRedirects = 5;

std::pair<std::string, std::string> split_url(const std::string& url) {
    auto scheme_end = url.find("://");
    if (scheme_end == std::string::npos)
        return {"", ""};
    auto host_start = scheme_end + 3;
    auto path_start = url.find('/', host_start);
    if (path_start == std::string::npos)
        return {url, "/"};
    return {url.substr(0, path_start), url.substr(path_start)};
}

/// Download a URL into an in-memory buffer. Returns true on success.
/// The redirect loop mirrors HfModelFetcher::download_with_redirects —
/// https-only, explicit downgrade refusal — but buffers the body
/// instead of streaming to disk because text lists fit comfortably
/// in memory at the kMaxListBytes cap.
bool download_buffered(const std::string& url, std::string& body, std::string& error) {
    std::string current = url;
    for (int hops = 0; hops < kMaxRedirects; ++hops) {
        if (current.rfind("https://", 0) != 0) {
            error = "refusing non-https url: " + current;
            return false;
        }
        auto [base, path] = split_url(current);
        if (base.empty()) {
            error = "malformed url: " + current;
            return false;
        }

        http::Client client(base);
        client.set_connection_timeout(10, 0);
        client.set_read_timeout(30, 0);

        int status = 0;
        std::string location;
        bool overflowed = false;
        body.clear();

        auto response_handler = [&](const http::Response& res) -> bool {
            status = res.status;
            if (res.status >= 300 && res.status < 400)
                location = res.get_header_value("Location");
            return true;
        };
        auto content_receiver = [&](const char* data, size_t data_length) -> bool {
            if (status >= 300 && status < 400)
                return true; // discard redirect body
            if (status != 200)
                return false;
            if (body.size() + data_length > kMaxListBytes) {
                overflowed = true;
                return false;
            }
            body.append(data, data_length);
            return true;
        };

        auto result = client.Get(path, response_handler, content_receiver);
        if (!result) {
            error = "transport failure fetching " + current;
            return false;
        }
        if (overflowed) {
            error = "list exceeds " + std::to_string(kMaxListBytes) + " byte cap";
            return false;
        }
        if (status >= 300 && status < 400) {
            if (location.empty()) {
                error = "redirect without Location from " + current;
                return false;
            }
            if (location.rfind("https://", 0) == 0) {
                current = location;
            }
            else if (location.rfind("http://", 0) == 0) {
                error = "refusing http:// redirect from " + current;
                return false;
            }
            else if (!location.empty() && location.front() == '/') {
                current = base + location;
            }
            else {
                error = "refusing non-absolute, non-https redirect: " + location;
                return false;
            }
            continue;
        }
        if (status != 200) {
            error = "http " + std::to_string(status) + " fetching " + current;
            return false;
        }
        if (body.empty()) {
            error = "empty response body from " + current;
            return false;
        }
        return true;
    }
    error = "too many redirects";
    return false;
}

/// Validate the caller-supplied cache filename. We refuse anything
/// that could escape cache_dir. This is the same rule HfModelFetcher
/// applies to HF-derived filenames: no separators, no `..`, printable
/// characters only. Operators pass these as constants in ServerSettings;
/// the check is insurance against a future caller that forwards a
/// user-supplied value.
bool is_safe_cache_name(const std::string& name) {
    if (name.empty() || name.size() > 128)
        return false;
    if (name.front() == '.' || name.back() == '.')
        return false;
    if (name.find("..") != std::string::npos)
        return false;
    for (char c : name) {
        auto u = static_cast<unsigned char>(c);
        if (!(std::isalnum(u) || c == '-' || c == '_' || c == '.'))
            return false;
    }
    return true;
}

/// Trim ASCII whitespace from both ends of a string view.
std::string_view trim(std::string_view s) {
    size_t i = 0;
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
        ++i;
    size_t j = s.size();
    while (j > i && std::isspace(static_cast<unsigned char>(s[j - 1])))
        --j;
    return s.substr(i, j - i);
}

} // namespace

std::vector<std::string> parse_text_list(std::string_view body) {
    std::vector<std::string> out;

    // Strip a leading UTF-8 BOM if present — S3 objects edited on
    // Windows boxes sometimes arrive with \xEF\xBB\xBF on the first
    // line, and "\xEF\xBB\xBF<word>" is a different token from
    // "<word>" as far as the word-boundary matcher is concerned.
    // Dropping the BOM at parse time keeps the first wordlist entry
    // from silently not-matching.
    if (body.size() >= 3 && static_cast<unsigned char>(body[0]) == 0xEF &&
        static_cast<unsigned char>(body[1]) == 0xBB && static_cast<unsigned char>(body[2]) == 0xBF) {
        body.remove_prefix(3);
    }

    size_t start = 0;
    while (start <= body.size()) {
        size_t end = body.find('\n', start);
        if (end == std::string_view::npos)
            end = body.size();

        std::string_view line = body.substr(start, end - start);
        // Drop trailing \r for CRLF files.
        if (!line.empty() && line.back() == '\r')
            line.remove_suffix(1);

        auto trimmed = trim(line);
        if (!trimmed.empty() && trimmed.front() != '#')
            out.emplace_back(trimmed);

        if (end == body.size())
            break;
        start = end + 1;
    }
    return out;
}

TextListFetchResult fetch_text_list(const std::string& url, const std::string& cache_dir,
                                    const std::string& cache_name) {
    TextListFetchResult out;

    if (url.empty()) {
        out.error = "empty url";
        return out;
    }
    if (!is_safe_cache_name(cache_name)) {
        out.error = "unsafe cache filename: " + cache_name;
        return out;
    }

    std::error_code ec;
    std::filesystem::create_directories(cache_dir, ec);
    if (ec) {
        out.error = "cannot create cache dir " + cache_dir + ": " + ec.message();
        return out;
    }
    const auto cache_path = std::filesystem::path(cache_dir) / cache_name;
    out.local_path = cache_path.string();

    // Try the fresh download first. On success, overwrite the cache.
    std::string body;
    std::string fetch_err;
    if (download_buffered(url, body, fetch_err)) {
        std::ofstream ofs(cache_path, std::ios::binary | std::ios::trunc);
        if (ofs) {
            ofs.write(body.data(), static_cast<std::streamsize>(body.size()));
            ofs.close();
        }
        else {
            // Non-fatal — we still have the bytes, just couldn't cache.
            Log::log_print(WARNING, "TextListFetcher: cannot write cache %s (continuing)", cache_path.string().c_str());
        }
        out.entries = parse_text_list(body);
        out.downloaded = true;
        out.ok = true;
        return out;
    }

    // Fresh fetch failed. Fall back to the cache file if one exists —
    // this is the "S3 is down during a restart" escape hatch. Without
    // it, a transient network blip would disable Layer 1c until the
    // next redeploy.
    if (std::filesystem::exists(cache_path)) {
        std::ifstream ifs(cache_path, std::ios::binary);
        if (ifs) {
            std::ostringstream ss;
            ss << ifs.rdbuf();
            std::string cached = ss.str();
            if (!cached.empty()) {
                out.entries = parse_text_list(cached);
                out.from_cache = true;
                out.ok = true;
                Log::log_print(WARNING, "TextListFetcher: using cached %s (%s)", cache_path.string().c_str(),
                               fetch_err.c_str());
                return out;
            }
        }
    }

    out.error = fetch_err;
    return out;
}

} // namespace moderation
