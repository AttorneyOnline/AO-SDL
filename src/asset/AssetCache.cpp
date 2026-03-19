#include "AssetCache.h"

AssetCache::AssetCache(size_t max_bytes) : max_bytes_(max_bytes) {}

std::shared_ptr<Asset> AssetCache::get(const std::string& path) {
    auto it = entries.find(path);
    if (it == entries.end()) return nullptr;

    // Promote to front of LRU list.
    lru.splice(lru.begin(), lru, it->second.lru_pos);

    return it->second.asset;
}

void AssetCache::insert(std::shared_ptr<Asset> asset) {
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

    evict_to_limit();
}

void AssetCache::evict_unused() {
    for (auto it = entries.begin(); it != entries.end();) {
        if (it->second.asset.use_count() == 1) {
            used_bytes_ -= it->second.asset->memory_size();
            lru.erase(it->second.lru_pos);
            it = entries.erase(it);
        }
        else {
            ++it;
        }
    }
}

void AssetCache::evict_to_limit() {
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
