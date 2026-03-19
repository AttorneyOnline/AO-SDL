#pragma once

#include "Asset.h"

#include <list>
#include <memory>
#include <string>
#include <unordered_map>

// LRU cache for loaded Asset objects.
//
// Callers receive shared_ptr<Asset>. As long as a caller holds that pointer,
// the asset is pinned in memory — the cache will not evict it even if it is
// the least recently used entry. Eviction only targets assets where
// use_count() == 1 (i.e. only the cache itself holds a reference).
//
// This mirrors the legacy client's "keep warm while in use" behavior.
// Transition/one-shot assets are naturally evicted once callers release them.
class AssetCache {
  public:
    // max_bytes: soft memory limit. The cache will try to evict LRU entries
    // when this limit is exceeded, skipping any that are still externally held.
    explicit AssetCache(size_t max_bytes);

    // Look up a cached asset by virtual path. Returns nullptr if not cached.
    std::shared_ptr<Asset> get(const std::string& path);

    // Insert a newly loaded asset. Triggers eviction if over the memory limit.
    void insert(std::shared_ptr<Asset> asset);

    // Evict all entries with no external holders (use_count == 1) regardless of limit.
    void evict_unused();

    size_t used_bytes() const { return m_used_bytes; }
    size_t max_bytes() const { return m_max_bytes; }

  private:
    void evict_to_limit();

    size_t m_max_bytes;
    size_t m_used_bytes = 0;

    // LRU ordering: front = most recently used, back = least recently used.
    using LruList = std::list<std::string>;
    LruList m_lru;

    struct Entry {
        std::shared_ptr<Asset> asset;
        LruList::iterator lru_pos;
    };

    std::unordered_map<std::string, Entry> m_entries;
};
