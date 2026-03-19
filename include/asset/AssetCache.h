/**
 * @file AssetCache.h
 * @brief LRU cache for loaded Asset objects with shared_ptr pinning.
 */
#pragma once

#include "Asset.h"

#include <list>
#include <memory>
#include <string>
#include <unordered_map>

/**
 * @brief LRU cache for loaded Asset objects.
 *
 * Callers receive shared_ptr<Asset>. As long as a caller holds that pointer,
 * the asset is pinned in memory -- the cache will not evict it even if it is
 * the least recently used entry. Eviction only targets assets where
 * use_count() == 1 (i.e. only the cache itself holds a reference).
 *
 * This mirrors the legacy client's "keep warm while in use" behavior.
 * Transition/one-shot assets are naturally evicted once callers release them.
 */
class AssetCache {
  public:
    /**
     * @brief Construct an AssetCache with a soft memory limit.
     * @param max_bytes Soft memory limit in bytes. The cache will try to evict
     *                  LRU entries when this limit is exceeded, skipping any
     *                  that are still externally held.
     */
    explicit AssetCache(size_t max_bytes);

    /**
     * @brief Look up a cached asset by virtual path.
     *
     * If found, the entry is promoted to the most-recently-used position.
     *
     * @param path The virtual asset path to look up.
     * @return A shared_ptr to the asset, or nullptr if not cached.
     */
    std::shared_ptr<Asset> get(const std::string& path);

    /**
     * @brief Insert a newly loaded asset into the cache.
     *
     * Triggers LRU eviction if the memory limit is exceeded after insertion.
     *
     * @param asset The asset to cache. Its path() is used as the cache key.
     */
    void insert(std::shared_ptr<Asset> asset);

    /**
     * @brief Evict all entries with no external holders regardless of the memory limit.
     *
     * Removes every entry whose shared_ptr use_count is 1 (only the cache holds it).
     */
    void evict_unused();

    /**
     * @brief Get the current total memory usage of cached assets.
     * @return Sum of memory_size() for all cached assets, in bytes.
     */
    size_t used_bytes() const { return used_bytes_; }

    /**
     * @brief Get the configured soft memory limit.
     * @return The maximum byte budget configured at construction.
     */
    size_t max_bytes() const { return max_bytes_; }

  private:
    /**
     * @brief Evict least-recently-used, unpinned entries until under the memory limit.
     */
    void evict_to_limit();

    size_t max_bytes_;
    size_t used_bytes_ = 0;

    /** @brief LRU ordering: front = most recently used, back = least recently used. */
    using LruList = std::list<std::string>;
    LruList lru;

    /** @brief Internal cache entry pairing an asset with its LRU list position. */
    struct Entry {
        std::shared_ptr<Asset> asset; /**< The cached asset. */
        LruList::iterator lru_pos;    /**< Iterator into the LRU list for O(1) promotion. */
    };

    std::unordered_map<std::string, Entry> entries; /**< Path-keyed lookup table. */
};
