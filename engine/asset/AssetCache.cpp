#include "asset/AssetCache.h"

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

void AssetCache::insert(std::shared_ptr<Asset> asset) {
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
    entries[path] = {asset, lru.begin()};

    evict_locked();
}

void AssetCache::evict() {
    std::lock_guard lock(mutex_);
    evict_locked();
}

void AssetCache::evict_locked() {
    // Walk LRU from back (least recently used), evicting only unpinned entries.
    auto it = lru.end();
    while (used_bytes_ > max_bytes_ && it != lru.begin()) {
        --it;
        auto entry_it = entries.find(*it);
        if (entry_it != entries.end() && entry_it->second.asset.use_count() == 1) {
            used_bytes_ -= entry_it->second.asset->memory_size();
            it = lru.erase(it);
            entries.erase(entry_it);
        }
    }
}
