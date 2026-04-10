#include "moderation/HfModelFetcher.h"

#include "net/Http.h"
#include "utils/Log.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace moderation {

namespace {

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

/// Default GGUF filename guess. Operators who want a specific quant
/// should use the "repo:filename" syntax. This is a last-resort guess
/// for bare ids.
std::string default_gguf_filename(const std::string& repo) {
    // Most HF GGUF repos name their files something like
    // "model-q4_k_m.gguf" or "ggml-model.gguf". We try a few.
    // For now, pick the first that exists via a HEAD request — but
    // we don't actually do that check here; the caller handles the
    // error path if the file doesn't exist.
    (void)repo;
    return "model.gguf";
}

/// HEAD request that follows redirects. httplib doesn't auto-follow
/// 302s, which is important because HF resolve URLs often redirect.
bool download_with_redirects(const std::string& url, const std::string& out_path, std::string& error) {
    std::string current = url;
    for (int hops = 0; hops < 5; ++hops) {
        auto scheme_end = current.find("://");
        if (scheme_end == std::string::npos) {
            error = "malformed url: " + current;
            return false;
        }
        auto host_start = scheme_end + 3;
        auto path_start = current.find('/', host_start);
        if (path_start == std::string::npos) {
            error = "no path in url: " + current;
            return false;
        }
        const std::string base = current.substr(0, path_start);
        const std::string path = current.substr(path_start);

        http::Client client(base);
        client.set_connection_timeout(10, 0);
        client.set_read_timeout(120, 0);
        // Follow redirects manually; httplib's built-in follow doesn't
        // always handle HF's cross-host redirects (cdn-lfs.huggingface.co).
        auto result = client.Get(path);
        if (!result) {
            error = "transport failure fetching " + current;
            return false;
        }
        if (result->status >= 300 && result->status < 400) {
            auto loc = result->get_header_value("Location");
            if (loc.empty()) {
                error = "redirect without Location from " + current;
                return false;
            }
            // Relative redirects stay on the same host.
            if (loc.rfind("http", 0) == 0)
                current = loc;
            else
                current = base + loc;
            continue;
        }
        if (result->status != 200) {
            error = "http " + std::to_string(result->status) + " fetching " + current;
            return false;
        }
        std::ofstream ofs(out_path, std::ios::binary);
        if (!ofs) {
            error = "cannot open " + out_path + " for write";
            return false;
        }
        ofs.write(result->body.data(), static_cast<std::streamsize>(result->body.size()));
        if (!ofs) {
            error = "write failed to " + out_path;
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

    // Case 2: HF repo id. Parse out the repo and optional filename.
    auto [repo, filename] = parse_id(hf_id);
    if (repo.empty()) {
        out.error = "empty hf id";
        return out;
    }
    if (filename.empty())
        filename = default_gguf_filename(repo);

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
