#include "moderation/HfModelFetcher.h"

#include "net/Http.h"
#include "utils/Log.h"

#include <cctype>
#include <filesystem>
#include <fstream>

namespace moderation {

namespace {

/// Maximum model file size we will accept. 2 GB is large enough for
/// a 1B-parameter Q8_0 GGUF (~1.2 GB) with headroom, but small enough
/// that a malicious or misconfigured HF URL can't OOM a t4g.small
/// during the download. Operators who want larger models should pick
/// a bigger instance AND bump this limit in a follow-up that makes
/// it configurable.
constexpr size_t kMaxModelBytes = 2ULL * 1024 * 1024 * 1024;

/// Allowed characters in an HF repo id. Reference:
/// https://huggingface.co/docs/hub/repositories-naming — owner and
/// repo are alphanumeric plus `-`, `_`, `.`, and the `/` that
/// separates them. We deliberately reject `..`, `?`, `#`, space,
/// control chars, and percent-encoding: these are the usual ways to
/// escape a URL path, add query parameters, or point at an unintended
/// resource. An attacker who can influence the HF model id config
/// value should NOT be able to reach arbitrary URLs through this
/// code path.
bool is_valid_repo_char(unsigned char c) {
    return std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '/';
}

bool is_valid_filename_char(unsigned char c) {
    return std::isalnum(c) || c == '-' || c == '_' || c == '.';
}

/// Validate a parsed repo id. Returns an empty string on success, an
/// error message on failure. Rules:
///   - Non-empty.
///   - Every char is alphanumeric or one of `-_./`.
///   - No `..` anywhere (path traversal).
///   - Does not start or end with `/` or `.`.
///   - Contains exactly one `/` (owner/repo form).
std::string validate_repo(const std::string& repo) {
    if (repo.empty())
        return "empty repo";
    if (repo.size() > 256)
        return "repo too long";
    if (repo.front() == '/' || repo.front() == '.' || repo.back() == '/' || repo.back() == '.')
        return "repo has leading/trailing '/' or '.'";
    if (repo.find("..") != std::string::npos)
        return "repo contains '..'";
    size_t slash_count = 0;
    for (char c : repo) {
        if (!is_valid_repo_char(static_cast<unsigned char>(c)))
            return "repo contains invalid character";
        if (c == '/')
            ++slash_count;
    }
    if (slash_count != 1)
        return "repo must be owner/name";
    return "";
}

std::string validate_filename(const std::string& filename) {
    if (filename.empty())
        return "empty filename";
    if (filename.size() > 256)
        return "filename too long";
    if (filename.front() == '.' || filename.back() == '.')
        return "filename has leading/trailing '.'";
    if (filename.find("..") != std::string::npos)
        return "filename contains '..'";
    for (char c : filename) {
        if (!is_valid_filename_char(static_cast<unsigned char>(c)))
            return "filename contains invalid character";
    }
    return "";
}

/// Sanitize an HF id into something safe to use as a filename.
/// Replaces '/' and ':' with '_' and strips anything that isn't
/// alphanumeric, dash, underscore, or dot.
std::string sanitize_filename(const std::string& id) {
    std::string out;
    out.reserve(id.size());
    for (char c : id) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.')
            out += c;
        else
            out += '_';
    }
    return out;
}

/// Split an id into {repo, filename} parts. If no ":filename" suffix
/// is present, returns {id, ""} and the caller must guess a default.
std::pair<std::string, std::string> parse_id(const std::string& id) {
    auto colon = id.find(':');
    if (colon == std::string::npos)
        return {id, ""};
    return {id.substr(0, colon), id.substr(colon + 1)};
}

/// Default GGUF filename guess for bare repo ids. Operators who want
/// a specific quant should use the `repo:filename` syntax — this is
/// a last-resort guess that'll probably 404.
std::string default_gguf_filename() {
    return "model.gguf";
}

/// Split a URL into {base (scheme://host), path}. Returns an empty
/// first on malformed input.
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

/// Streaming download with a manual redirect follower.
///
/// We use httplib's ContentReceiver callback instead of letting the
/// Result buffer the entire body in memory. The receiver writes each
/// chunk directly to disk and enforces a hard byte cap
/// (kMaxModelBytes); if the cap is exceeded the callback returns
/// false, httplib aborts the request, and the partial file is removed
/// by the caller.
///
/// Redirects are followed manually with an explicit https:// scheme
/// check on the new location, so a 302 pointing at
/// file:// / gopher:// / an http:// downgrade cannot succeed.
bool download_with_redirects(const std::string& url, const std::string& out_path, std::string& error) {
    std::string current = url;
    for (int hops = 0; hops < 5; ++hops) {
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
        client.set_read_timeout(120, 0);

        std::ofstream ofs(out_path, std::ios::binary);
        if (!ofs) {
            error = "cannot open " + out_path + " for write";
            return false;
        }
        size_t total_bytes = 0;
        bool overflowed = false;

        // Response header receiver — capture status + Location if
        // this turns out to be a redirect. We still need it because
        // httplib's ContentReceiver fires for 3xx bodies too.
        int status = 0;
        std::string location;
        auto response_handler = [&](const http::Response& res) -> bool {
            status = res.status;
            if (res.status >= 300 && res.status < 400) {
                location = res.get_header_value("Location");
            }
            // Continue to content receiver either way; we skip writing
            // for 3xx by checking `status` inside the content lambda.
            return true;
        };
        auto content_receiver = [&](const char* data, size_t data_length) -> bool {
            if (status >= 300 && status < 400)
                return true; // ignore redirect body
            if (status != 200)
                return false; // stop on unexpected status
            if (total_bytes + data_length > kMaxModelBytes) {
                overflowed = true;
                return false;
            }
            ofs.write(data, static_cast<std::streamsize>(data_length));
            if (!ofs)
                return false;
            total_bytes += data_length;
            return true;
        };

        auto result = client.Get(path, response_handler, content_receiver);
        ofs.close();

        if (!result) {
            error = "transport failure fetching " + current;
            return false;
        }
        if (overflowed) {
            error = "model exceeds " + std::to_string(kMaxModelBytes) + " byte cap";
            return false;
        }
        if (status >= 300 && status < 400) {
            if (location.empty()) {
                error = "redirect without Location from " + current;
                return false;
            }
            if (location.rfind("https://", 0) == 0) {
                current = location; // absolute https redirect
            }
            else if (location.rfind("http://", 0) == 0) {
                // Downgrade attack — refuse.
                error = "refusing http:// redirect from " + current;
                return false;
            }
            else if (!location.empty() && location.front() == '/') {
                // Relative redirect — stays on the same base (which is
                // https because we checked at the top of the loop).
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
        if (total_bytes == 0) {
            error = "empty response body from " + current;
            return false;
        }
        return true;
    }
    error = "too many redirects";
    return false;
}

} // namespace

HfFetchResult resolve_hf_model(const std::string& hf_id, const std::string& cache_dir) {
    HfFetchResult out;

    // Case 1: already a local filesystem path.
    if (std::filesystem::exists(hf_id) && std::filesystem::is_regular_file(hf_id)) {
        out.ok = true;
        out.local_path = hf_id;
        return out;
    }

    // Case 2: HF repo id. Parse out the repo and optional filename,
    // and validate both BEFORE using them to build a URL. This is the
    // security boundary: the hf_id comes from operator config, which
    // is trusted today but may be sourced from less-trusted places
    // in the future (e.g. a stack parameter supplied by a CI job).
    auto [repo, filename] = parse_id(hf_id);
    if (auto err = validate_repo(repo); !err.empty()) {
        out.error = "invalid hf repo: " + err;
        return out;
    }
    if (filename.empty())
        filename = default_gguf_filename();
    if (auto err = validate_filename(filename); !err.empty()) {
        out.error = "invalid hf filename: " + err;
        return out;
    }

    // Cache path: cache_dir / sanitized(repo)_filename.
    std::filesystem::path cache = cache_dir;
    std::error_code ec;
    std::filesystem::create_directories(cache, ec);
    if (ec) {
        out.error = "cannot create cache dir " + cache_dir + ": " + ec.message();
        return out;
    }
    const std::string cached_name = sanitize_filename(repo) + "_" + sanitize_filename(filename);
    const auto target = cache / cached_name;

    if (std::filesystem::exists(target)) {
        out.ok = true;
        out.local_path = target.string();
        Log::log_print(INFO, "HfModelFetcher: cache hit %s", out.local_path.c_str());
        return out;
    }

    // Build the HF resolve URL and download.
    const std::string url = "https://huggingface.co/" + repo + "/resolve/main/" + filename;
    Log::log_print(INFO, "HfModelFetcher: downloading %s -> %s", url.c_str(), target.string().c_str());
    if (!download_with_redirects(url, target.string(), out.error)) {
        // Clean up any partial file.
        std::filesystem::remove(target, ec);
        return out;
    }

    out.ok = true;
    out.local_path = target.string();
    out.downloaded = true;
    return out;
}

} // namespace moderation
