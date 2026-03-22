#include "asset/MountManager.h"

#include "asset/MountArchive.h"
#include "asset/MountDirectory.h"
#include "asset/MountEmbedded.h"
#include "asset/MountHttp.h"
#include "utils/Log.h"

#include <format>
#include <shared_mutex>

MountManager::MountManager() {
    // Embedded assets always available, even before load_mounts is called
    auto embedded = std::make_unique<MountEmbedded>();
    embedded->load();
    loaded_mounts.push_back(std::move(embedded));
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
            loaded_mounts.push_back(std::move(mount));
        }
        catch (const std::exception& e) {
            Log::log_print(WARNING,
                           std::format("Failed to create mount at {}: {}", mount_path.string(), e.what()).c_str());
        }
    }

    // Embedded assets after local mounts, before HTTP
    auto embedded = std::make_unique<MountEmbedded>();
    embedded->load();
    loaded_mounts.push_back(std::move(embedded));
}

void MountManager::add_mount(std::unique_ptr<Mount> mount) {
    std::unique_lock<std::shared_mutex> locker(lock);
    mount->load();
    loaded_mounts.push_back(std::move(mount));
}

std::vector<std::string> MountManager::http_extensions(int asset_type) const {
    std::shared_lock<std::shared_mutex> locker(lock);
    for (auto& mount : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(mount.get());
        if (http && http->has_extensions())
            return http->extensions_for(static_cast<MountHttp::AssetType>(asset_type));
    }
    return {};
}

void MountManager::prefetch(const std::string& relative_path, int priority) {
    std::shared_lock<std::shared_mutex> locker(lock);

    // If any local (non-HTTP) mount has the file, skip — no need to fetch remotely
    for (auto& mount : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(mount.get());
        if (!http && mount->seek_file(relative_path))
            return;
    }

    // Trigger HTTP downloads — only hit fallback mounts if the primary
    // mount already failed for this path (avoids duplicate 404 floods).
    MountHttp* primary = nullptr;
    for (auto& mount : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(mount.get());
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
    for (auto& mount : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(mount.get());
        if (http)
            http->pool().drop_below(static_cast<HttpPriority>(priority));
    }
}

MountManager::HttpStats MountManager::http_stats() const {
    std::shared_lock<std::shared_mutex> locker(lock);
    HttpStats stats;
    for (auto& mount : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(mount.get());
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
    for (auto& mount : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(mount.get());
        if (http) {
            for (const auto& e : http->cache_snapshot())
                result.push_back({e.path, e.bytes});
        }
    }
    return result;
}

void MountManager::release_all_http() {
    std::shared_lock<std::shared_mutex> locker(lock);
    for (auto& mount : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(mount.get());
        if (http)
            http->release_all();
    }
}

bool MountManager::fetch_streaming(const std::string& relative_path,
                                   std::function<bool(const uint8_t*, size_t)> on_chunk) {
    std::shared_lock<std::shared_mutex> locker(lock);
    for (auto& mount : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(mount.get());
        if (!http)
            continue;
        if (http->fetch_streaming(relative_path, on_chunk))
            return true;
    }
    return false;
}

void MountManager::release_http(const std::string& relative_path) {
    std::shared_lock<std::shared_mutex> locker(lock);
    for (auto& mount : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(mount.get());
        if (http)
            http->release(relative_path);
    }
}

std::optional<std::vector<uint8_t>> MountManager::fetch_data(const std::string& relative_path) {
    std::shared_lock<std::shared_mutex> locker(lock);

    for (auto& mount : loaded_mounts) {
        if (mount->seek_file(relative_path)) {
            try {
                return mount->fetch_data(relative_path);
            }
            catch (const std::exception& e) {
                Log::log_print(ERR, std::format("Failed to fetch {}: {}", relative_path, e.what()).c_str());
            }
        }
    }

    // Sync fallback: try HTTP mounts for config files that must be available
    // immediately (e.g. char.ini needed before emotes can load).
    // Audio files are NOT included — the AudioThread handles those via
    // background download + streaming to avoid blocking the game thread.
    if (relative_path.ends_with(".ini") || relative_path.ends_with(".json")) {
        for (auto& mount : loaded_mounts) {
            auto* http = dynamic_cast<MountHttp*>(mount.get());
            if (!http)
                continue;
            auto data = http->fetch_sync(relative_path);
            if (!data.empty())
                return data;
        }
    }

    return std::nullopt;
}
