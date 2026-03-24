#include "asset/AssetCache.h"

#include "utils/Log.h"

AssetCache::AssetCache(size_t max_bytes) : max_bytes_(max_bytes) {
}

std::shared_ptr<Asset> AssetCache::get(const std::string& path) {
    std::lock_guard lock(mutex_);
    auto it = entries.find(path);
    if (it == entries.end())
        return nullptr;

    // Promote to front of LRU list.
    lru.splice(lru.begin(), lru, it->second.lru_pos);

    return it->second.asset;
}

std::shared_ptr<Asset> AssetCache::peek(const std::string& path) const {
    std::lock_guard lock(mutex_);
    auto it = entries.find(path);
    if (it == entries.end())
        return nullptr;
    return it->second.asset;
}

void AssetCache::insert(std::shared_ptr<Asset> asset, uint32_t session_id) {
    std::lock_guard lock(mutex_);
    const std::string& path = asset->path();

    // If already cached, remove the old entry's accounting first.
    auto it = entries.find(path);
    if (it != entries.end()) {
        used_bytes_ -= it->second.asset->memory_size();
        lru.erase(it->second.lru_pos);
        entries.erase(it);
    }

    used_bytes_ += asset->memory_size();
    lru.push_front(path);
    entries[path] = {asset, lru.begin(), session_id};

    evict_locked();
}

void AssetCache::evict() {
    std::lock_guard lock(mutex_);
    evict_locked();
}

void AssetCache::clear() {
    std::lock_guard lock(mutex_);
    Log::log_print(INFO, "AssetCache: clearing all entries (%zu entries, %zuMB)", entries.size(),
                   used_bytes_ / (1024 * 1024));
    entries.clear();
    lru.clear();
    used_bytes_ = 0;
}

void AssetCache::clear_session(uint32_t session_id) {
    if (session_id == 0)
        return;
    std::lock_guard lock(mutex_);
    int removed = 0;
    auto it = lru.begin();
    while (it != lru.end()) {
        auto entry_it = entries.find(*it);
        if (entry_it != entries.end() && entry_it->second.session_id == session_id) {
            used_bytes_ -= entry_it->second.asset->memory_size();
            entries.erase(entry_it);
            it = lru.erase(it);
            removed++;
        }
        else {
            ++it;
        }
    }
    if (removed > 0)
        Log::log_print(INFO, "AssetCache: cleared %d session %u entries", removed, session_id);
}

void AssetCache::evict_locked() {
    if (used_bytes_ <= max_bytes_)
        return;

    size_t before = used_bytes_;
    int evicted = 0;
    int skipped = 0;

    // Walk LRU from back (least recently used), evicting only unpinned entries.
    auto it = lru.end();
    while (used_bytes_ > max_bytes_ && it != lru.begin()) {
        --it;
        auto entry_it = entries.find(*it);
        if (entry_it != entries.end()) {
            long refs = entry_it->second.asset.use_count();
            if (refs == 1) {
                Log::log_print(DEBUG, "AssetCache: evict '%s' (%zu bytes, refs=%ld)", it->c_str(),
                               entry_it->second.asset->memory_size(), refs);
                used_bytes_ -= entry_it->second.asset->memory_size();
                it = lru.erase(it);
                entries.erase(entry_it);
                evicted++;
            }
            else {
                skipped++;
            }
        }
    }

    if (evicted > 0 || skipped > 0) {
        Log::log_print(DEBUG, "AssetCache: eviction pass: %d evicted, %d pinned, %zuMB -> %zuMB (limit %zuMB)", evicted,
                       skipped, before / (1024 * 1024), used_bytes_ / (1024 * 1024), max_bytes_ / (1024 * 1024));
    }
}
