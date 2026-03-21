#include "asset/MountHttp.h"

#include "utils/Log.h"

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
    } else {
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
    // Nothing to index — files are discovered on demand.
}

bool MountHttp::seek_file(const std::string& path) const {
    std::lock_guard lock(mutex_);
    return cache_.find(path) != cache_.end();
}

std::vector<uint8_t> MountHttp::fetch_data(const std::string& path) {
    std::lock_guard lock(mutex_);
    auto it = cache_.find(path);
    if (it != cache_.end())
        return it->second;
    return {};
}

void MountHttp::request(const std::string& path) {
    {
        std::lock_guard lock(mutex_);
        // Already have it, downloading it, or know it doesn't exist
        if (cache_.count(path) || pending_.count(path) || failed_.count(path))
            return;
        pending_.insert(path);
    }

    std::string http_path = path_prefix_ + "/" + path;
    std::string captured_path = path;

    pool_.get(host_, http_path, [this, captured_path](HttpResponse resp) {
        std::lock_guard lock(mutex_);
        pending_.erase(captured_path);

        if (resp.status == 200 && !resp.body.empty()) {
            cache_[captured_path] = std::vector<uint8_t>(resp.body.begin(), resp.body.end());
            Log::log_print(VERBOSE, "MountHttp: downloaded %s (%zu bytes)",
                           captured_path.c_str(), resp.body.size());
        } else {
            failed_.insert(captured_path);
            Log::log_print(VERBOSE, "MountHttp: failed %s (status=%d)",
                           captured_path.c_str(), resp.status);
        }
    });
}

int MountHttp::pending_count() const {
    std::lock_guard lock(mutex_);
    return (int)pending_.size();
}
