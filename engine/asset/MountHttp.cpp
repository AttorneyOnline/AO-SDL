#include "asset/MountHttp.h"

#include "utils/Log.h"

#include <httplib.h>
#include <json.hpp>

#include <algorithm>
#include <iomanip>
#include <sstream>

/// Lowercase a path — AO2 asset servers store files lowercase.
static std::string lowercase_path(const std::string& path) {
    std::string out = path;
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

/// Percent-encode a URL path component (spaces, parens, unicode, etc.).
static std::string url_encode_path(const std::string& path) {
    std::ostringstream out;
    out.fill('0');
    out << std::hex;
    for (unsigned char c : path) {
        // Unreserved characters per RFC 3986 + '/' for path separators
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/' || c == '(' || c == ')') {
            out << c;
        }
        else {
            out << '%' << std::uppercase << std::setw(2) << (int)c;
        }
    }
    return out.str();
}

/// Split a URL like "https://host.com/path/prefix" into host and path parts.
static void split_url(const std::string& url, std::string& host, std::string& path_prefix) {
    // Find the scheme + authority boundary (after "://")
    size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        host = url;
        path_prefix = "";
        return;
    }
    // Find the first '/' after the authority
    size_t path_start = url.find('/', scheme_end + 3);
    if (path_start == std::string::npos) {
        host = url;
        path_prefix = "";
    }
    else {
        host = url.substr(0, path_start);
        path_prefix = url.substr(path_start);
        // Remove trailing slash
        if (!path_prefix.empty() && path_prefix.back() == '/')
            path_prefix.pop_back();
    }
}

MountHttp::MountHttp(const std::string& base_url, HttpPool& pool)
    : Mount(std::filesystem::path(base_url)), pool_(pool) {
    split_url(base_url, host_, path_prefix_);
    base_url_ = base_url;
    // Ensure no trailing slash on base_url_
    if (!base_url_.empty() && base_url_.back() == '/')
        base_url_.pop_back();
    Log::log_print(DEBUG, "MountHttp: host=%s prefix=%s", host_.c_str(), path_prefix_.c_str());
}

void MountHttp::load() {
    // Fetch extensions.json to know which file formats the server hosts
    fetch_extensions();
}

void MountHttp::fetch_extensions() {
    std::string ext_path = path_prefix_ + "/extensions.json";
    pool_.get(
        host_, url_encode_path(ext_path),
        [this](HttpResponse resp) {
            if (resp.status != 200 || resp.body.empty()) {
                Log::log_print(DEBUG, "MountHttp: no extensions.json, using defaults");
                return;
            }
            try {
                auto j = nlohmann::json::parse(resp.body);
                auto parse_list = [](const nlohmann::json& j, const std::string& key) -> std::vector<std::string> {
                    std::vector<std::string> out;
                    if (j.contains(key) && j[key].is_array()) {
                        for (const auto& v : j[key]) {
                            std::string ext = v.get<std::string>();
                            // Strip leading dot: ".png" -> "png"
                            if (!ext.empty() && ext[0] == '.')
                                ext = ext.substr(1);
                            // Skip compound extensions like "webp.static"
                            if (ext.find('.') != std::string::npos)
                                continue;
                            out.push_back(ext);
                        }
                    }
                    return out;
                };

                std::lock_guard lock(mutex_);
                charicon_exts_ = parse_list(j, "charicon_extensions");
                emote_exts_ = parse_list(j, "emote_extensions");
                emotions_exts_ = parse_list(j, "emotions_extensions");
                background_exts_ = parse_list(j, "background_extensions");
                extensions_loaded_ = true;

                Log::log_print(DEBUG, "MountHttp: extensions.json loaded (icons=%zu emotes=%zu bg=%zu)",
                               charicon_exts_.size(), emote_exts_.size(), background_exts_.size());
            }
            catch (const std::exception& e) {
                Log::log_print(WARNING, "MountHttp: failed to parse extensions.json: %s", e.what());
            }
        },
        HttpPriority::CRITICAL);
}

std::vector<std::string> MountHttp::extensions_for(AssetType type) const {
    std::lock_guard lock(mutex_);
    if (extensions_loaded_) {
        switch (type) {
        case AssetType::CHARICON:
            return charicon_exts_;
        case AssetType::EMOTE:
            return emote_exts_;
        case AssetType::EMOTIONS:
            return emotions_exts_;
        case AssetType::BACKGROUND:
            return background_exts_;
        }
    }
    // Defaults
    return {"webp", "png", "gif"};
}

bool MountHttp::has_extensions() const {
    std::lock_guard lock(mutex_);
    return extensions_loaded_;
}

bool MountHttp::seek_file(const std::string& path) const {
    std::lock_guard lock(mutex_);
    return cache_.find(lowercase_path(path)) != cache_.end();
}

std::vector<uint8_t> MountHttp::fetch_data(const std::string& path) {
    std::lock_guard lock(mutex_);
    auto it = cache_.find(lowercase_path(path));
    if (it != cache_.end())
        return it->second;
    return {};
}

std::vector<uint8_t> MountHttp::fetch_sync(const std::string& raw_path) {
    std::string path = lowercase_path(raw_path);
    // Check cache first
    {
        std::lock_guard lock(mutex_);
        auto it = cache_.find(path);
        if (it != cache_.end())
            return it->second;
        if (failed_.count(path))
            return {};
    }

    // Blocking HTTP GET — short timeouts to avoid stalling the game
    std::string http_path = url_encode_path(path_prefix_ + "/" + path);
    httplib::Client cli(host_);
    cli.set_connection_timeout(2);
    cli.set_read_timeout(3);
    cli.set_follow_location(false); // Don't follow redirects (301s waste time)
    auto res = cli.Get(http_path);

    if (res && res->status == 200 && !res->body.empty()) {
        std::vector<uint8_t> data(res->body.begin(), res->body.end());
        Log::log_print(VERBOSE, "MountHttp: sync downloaded %s (%zu bytes)", path.c_str(), data.size());
        std::lock_guard lock(mutex_);
        cache_[path] = data;
        // Remove from pending so any async request in-flight won't re-store
        pending_.erase(path);
        return data;
    }

    std::lock_guard lock(mutex_);
    failed_.insert(path);
    pending_.erase(path);
    return {};
}

bool MountHttp::fetch_streaming(const std::string& raw_path, std::function<bool(const uint8_t*, size_t)> on_chunk) {
    std::string path = lowercase_path(raw_path);
    {
        std::lock_guard lock(mutex_);
        if (failed_.count(path))
            return false;
        // If already cached, deliver it all at once
        auto it = cache_.find(path);
        if (it != cache_.end()) {
            on_chunk(it->second.data(), it->second.size());
            return true;
        }
    }

    std::string http_path = url_encode_path(path_prefix_ + "/" + path);
    httplib::Client cli(host_);
    cli.set_connection_timeout(5);
    cli.set_read_timeout(30);

    bool success = false;
    auto res = cli.Get(http_path, [&](const char* data, size_t len) -> bool {
        success = true;
        return on_chunk(reinterpret_cast<const uint8_t*>(data), len);
    });

    if (!res || res->status != 200) {
        std::lock_guard lock(mutex_);
        failed_.insert(path);
        return false;
    }

    Log::log_print(VERBOSE, "MountHttp: streamed %s", path.c_str());
    return success;
}

void MountHttp::request(const std::string& raw_path, HttpPriority priority) {
    std::string path = lowercase_path(raw_path);
    {
        std::lock_guard lock(mutex_);
        // Already have it, downloading it, or know it doesn't exist
        if (cache_.count(path) || pending_.count(path) || failed_.count(path))
            return;
        pending_.insert(path);
    }

    std::string http_path = url_encode_path(path_prefix_ + "/" + path);
    std::string captured_path = path;

    pool_.get(
        host_, http_path,
        [this, captured_path](HttpResponse resp) {
            std::lock_guard lock(mutex_);

            // If not in pending_, fetch_sync already handled this path — skip
            if (!pending_.count(captured_path))
                return;
            pending_.erase(captured_path);

            if (resp.status == 200 && !resp.body.empty()) {
                cache_[captured_path] = std::vector<uint8_t>(resp.body.begin(), resp.body.end());
                Log::log_print(VERBOSE, "MountHttp: downloaded %s (%zu bytes)", captured_path.c_str(),
                               resp.body.size());
            }
            else if (resp.error == "dropped") {
                // Request was dropped by priority — don't mark as failed so it can be re-requested
            }
            else {
                failed_.insert(captured_path);
                Log::log_print(VERBOSE, "MountHttp: failed %s (status=%d err=%s)", captured_path.c_str(), resp.status,
                               resp.error.c_str());
            }
        },
        priority);
}

int MountHttp::pending_count() const {
    std::lock_guard lock(mutex_);
    return (int)pending_.size();
}

int MountHttp::cached_count() const {
    std::lock_guard lock(mutex_);
    return (int)cache_.size();
}

int MountHttp::failed_count() const {
    std::lock_guard lock(mutex_);
    return (int)failed_.size();
}

bool MountHttp::has_failed(const std::string& path) const {
    std::lock_guard lock(mutex_);
    return failed_.count(lowercase_path(path)) > 0;
}

void MountHttp::release(const std::string& path) {
    std::lock_guard lock(mutex_);
    cache_.erase(lowercase_path(path));
}

void MountHttp::release_all() {
    std::lock_guard lock(mutex_);
    cache_.clear();
}

std::vector<MountHttp::CacheEntry> MountHttp::cache_snapshot() const {
    std::lock_guard lock(mutex_);
    std::vector<CacheEntry> result;
    result.reserve(cache_.size());
    for (const auto& [path, data] : cache_)
        result.push_back({path, data.size()});
    return result;
}

size_t MountHttp::cached_bytes() const {
    std::lock_guard lock(mutex_);
    size_t total = 0;
    for (const auto& [_, data] : cache_)
        total += data.size();
    return total;
}
