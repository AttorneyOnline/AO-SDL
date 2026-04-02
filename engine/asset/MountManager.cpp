#include "asset/MountManager.h"

#include "asset/MountArchive.h"
#include "asset/MountDirectory.h"
#include "asset/MountEmbedded.h"
#include "asset/MountHttp.h"
#include "utils/Log.h"

#include <algorithm>
#include <format>
#include <future>
#include <shared_mutex>

MountManager::MountManager() {
    // Embedded assets always available, even before load_mounts is called
    auto embedded = std::make_unique<MountEmbedded>();
    embedded->load();
    loaded_mounts.push_back({0, std::move(embedded), 100});
}

void MountManager::load_mounts(const std::vector<std::filesystem::path>& target_mount_path) {
    std::unique_lock<std::shared_mutex> locker(lock);

    loaded_mounts.clear();

    // Priority: disk → archive → embedded → (HTTP added later via add_mount)
    for (const std::filesystem::path& mount_path : target_mount_path) {
        try {
            std::unique_ptr<Mount> mount;

            if (std::filesystem::is_directory(mount_path)) {
                mount = std::make_unique<MountDirectory>(mount_path);
            }
            else {
                mount = std::make_unique<MountArchive>(mount_path);
            }

            mount->load();
            loaded_mounts.push_back({0, std::move(mount), 0});
        }
        catch (const std::exception& e) {
            Log::log_print(WARNING,
                           std::format("Failed to create mount at {}: {}", mount_path.string(), e.what()).c_str());
        }
    }

    // Embedded assets after local mounts, before HTTP
    auto embedded = std::make_unique<MountEmbedded>();
    embedded->load();
    loaded_mounts.push_back({0, std::move(embedded), 100});
}

MountManager::MountHandle MountManager::add_mount(std::unique_ptr<Mount> mount, int priority) {
    std::unique_lock<std::shared_mutex> locker(lock);
    mount->load();
    MountHandle handle = next_handle_++;
    // Insert before the first entry with strictly greater priority (stable within same priority).
    auto pos = std::find_if(loaded_mounts.begin(), loaded_mounts.end(),
                            [priority](const MountEntry& e) { return e.priority > priority; });
    loaded_mounts.insert(pos, {handle, std::move(mount), priority});
    return handle;
}

void MountManager::remove_mount(MountHandle handle) {
    std::unique_lock<std::shared_mutex> locker(lock);
    auto it = std::remove_if(loaded_mounts.begin(), loaded_mounts.end(),
                             [handle](const MountEntry& e) { return e.handle == handle; });
    if (it != loaded_mounts.end()) {
        loaded_mounts.erase(it, loaded_mounts.end());
        Log::log_print(DEBUG, "MountManager: removed mount handle %u", handle);
    }
}

std::vector<std::string> MountManager::http_extensions(int asset_type) const {
    std::shared_lock<std::shared_mutex> locker(lock);
    for (auto& entry : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(entry.mount.get());
        if (http && http->has_extensions())
            return http->extensions_for(static_cast<MountHttp::AssetType>(asset_type));
    }
    return {};
}

void MountManager::prefetch(const std::string& relative_path, int priority) {
    std::shared_lock<std::shared_mutex> locker(lock);

    // If any local (non-HTTP) mount has the file, skip — no need to fetch remotely
    for (auto& entry : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(entry.mount.get());
        if (!http && entry.mount->seek_file(relative_path))
            return;
    }

    // Trigger HTTP downloads — only hit fallback mounts if the primary
    // mount already failed for this path (avoids duplicate 404 floods).
    MountHttp* primary = nullptr;
    for (auto& entry : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(entry.mount.get());
        if (!http)
            continue;
        if (!primary) {
            // First HTTP mount is the primary — always try it
            primary = http;
            http->request(relative_path, static_cast<HttpPriority>(priority));
        }
        else if (primary->has_failed(relative_path)) {
            // Primary failed — try fallback mounts
            http->request(relative_path, static_cast<HttpPriority>(priority));
        }
    }
}

void MountManager::drop_http_below(int priority) {
    std::shared_lock<std::shared_mutex> locker(lock);
    for (auto& entry : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(entry.mount.get());
        if (http)
            http->pool().drop_below(static_cast<HttpPriority>(priority));
    }
}

uint32_t MountManager::http_cache_generation() const {
    std::shared_lock<std::shared_mutex> locker(lock);
    uint32_t gen = 0;
    for (auto& entry : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(entry.mount.get());
        if (http)
            gen += http->cache_generation();
    }
    return gen;
}

MountManager::HttpStats MountManager::http_stats() const {
    std::shared_lock<std::shared_mutex> locker(lock);
    HttpStats stats;
    for (auto& entry : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(entry.mount.get());
        if (http) {
            stats.pending += http->pending_count();
            stats.cached += http->cached_count();
            stats.failed += http->failed_count();
            stats.cached_bytes += http->cached_bytes();
            stats.pool_pending = http->pool().pending();
        }
    }
    return stats;
}

std::vector<MountManager::HttpCacheEntry> MountManager::http_cache_snapshot() const {
    std::shared_lock<std::shared_mutex> locker(lock);
    std::vector<HttpCacheEntry> result;
    for (auto& entry : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(entry.mount.get());
        if (http) {
            for (const auto& e : http->cache_snapshot())
                result.push_back({e.path, e.bytes});
        }
    }
    return result;
}

void MountManager::release_all_http() {
    std::shared_lock<std::shared_mutex> locker(lock);
    for (auto& entry : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(entry.mount.get());
        if (http)
            http->release_all();
    }
}

bool MountManager::fetch_streaming(const std::string& relative_path,
                                   std::function<bool(const uint8_t*, size_t)> on_chunk) {
    std::shared_lock<std::shared_mutex> locker(lock);
    for (auto& entry : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(entry.mount.get());
        if (!http)
            continue;
        if (http->fetch_streaming(relative_path, on_chunk))
            return true;
    }
    return false;
}

bool MountManager::fetch_streaming_url(const std::string& url, std::function<bool(const uint8_t*, size_t)> on_chunk) {
    // Parse URL into host + path: "https://example.com/path/to/file.opus"
    size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos)
        return false;
    size_t path_start = url.find('/', scheme_end + 3);
    if (path_start == std::string::npos)
        return false;

    std::string host = url.substr(0, path_start);
    std::string path = url.substr(path_start);

    // Find any HTTP mount to borrow its pool for the connection
    std::shared_lock<std::shared_mutex> locker(lock);
    for (auto& entry : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(entry.mount.get());
        if (!http)
            continue;

        std::promise<bool> done;
        auto future = done.get_future();
        bool got_data = false;

        http->pool().get_streaming(
            host, path,
            [&](const uint8_t* data, size_t len) -> bool {
                got_data = true;
                return on_chunk(data, len);
            },
            [&](HttpResponse resp) {
                if (resp.status == 200 && got_data) {
                    Log::log_print(VERBOSE, "MountManager: streamed URL %s", url.c_str());
                    done.set_value(true);
                }
                else {
                    Log::log_print(VERBOSE, "MountManager: URL stream failed %s (status=%d err=%s)", url.c_str(),
                                   resp.status, resp.error.c_str());
                    done.set_value(false);
                }
            },
            HttpPriority::HIGH);

        return future.get();
    }

    Log::log_print(WARNING, "MountManager: no HTTP mount available for URL streaming: %s", url.c_str());
    return false;
}

void MountManager::release_http(const std::string& relative_path) {
    std::shared_lock<std::shared_mutex> locker(lock);
    for (auto& entry : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(entry.mount.get());
        if (http)
            http->release(relative_path);
    }
}

std::optional<std::vector<uint8_t>> MountManager::fetch_data(const std::string& relative_path) {
    std::shared_lock<std::shared_mutex> locker(lock);

    for (auto& entry : loaded_mounts) {
        try {
            auto data = entry.mount->try_fetch(relative_path);
            if (!data.empty())
                return data;
        }
        catch (const std::exception& e) {
            Log::log_print(ERR, std::format("Failed to fetch {}: {}", relative_path, e.what()).c_str());
        }
    }

    // If a config file wasn't found in any mount (including HTTP cache),
    // trigger an async prefetch so it'll be available on a future call.
    if (relative_path.ends_with(".ini") || relative_path.ends_with(".json")) {
        for (auto& entry : loaded_mounts) {
            auto* http = dynamic_cast<MountHttp*>(entry.mount.get());
            if (http && !http->has_failed(relative_path)) {
                http->request(relative_path, HttpPriority::HIGH);
            }
        }
    }

    return std::nullopt;
}
