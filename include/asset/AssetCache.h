/**
 * @file AssetCache.h
 * @brief LRU cache for loaded Asset objects with shared_ptr pinning.
 */
#pragma once

#include "Asset.h"

#include <list>
#include <memory>
#include <mutex>
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

    /// Look up without promoting in the LRU list (read-only peek).
    std::shared_ptr<Asset> peek(const std::string& path) const;

    /**
     * @brief Insert a newly loaded asset into the cache.
     *
     * Triggers LRU eviction if the memory limit is exceeded after insertion.
     *
     * @param asset The asset to cache. Its path() is used as the cache key.
     */
    void insert(std::shared_ptr<Asset> asset);

    /**
     * @brief Evict LRU entries that are not externally held until under the memory limit.
     *
     * Call periodically as a nudge — only evicts when the cache is over budget.
     * Entries with use_count > 1 (held by callers) are skipped.
     */
    void evict();

    /**
     * @brief Get the current total memory usage of cached assets.
     * @return Sum of memory_size() for all cached assets, in bytes.
     */
    size_t used_bytes() const {
        std::lock_guard lock(mutex_);
        return used_bytes_;
    }

    /**
     * @brief Get the configured soft memory limit.
     * @return The maximum byte budget configured at construction.
     */
    size_t max_bytes() const {
        return max_bytes_;
    }

    size_t entry_count() const {
        std::lock_guard lock(mutex_);
        return entries.size();
    }

    struct CacheEntry {
        std::string path;
        std::string format;
        size_t bytes;
        long use_count;
    };

    /// Snapshot in insertion order (unordered).
    std::vector<CacheEntry> snapshot() const {
        std::lock_guard lock(mutex_);
        std::vector<CacheEntry> result;
        result.reserve(entries.size());
        for (const auto& [path, entry] : entries) {
            result.push_back({path, entry.asset->format(), entry.asset->memory_size(), entry.asset.use_count()});
        }
        return result;
    }

    /// Snapshot in LRU order (front = most recently used).
    std::vector<CacheEntry> snapshot_lru() const {
        std::lock_guard lock(mutex_);
        std::vector<CacheEntry> result;
        result.reserve(lru.size());
        for (const auto& path : lru) {
            auto it = entries.find(path);
            if (it != entries.end())
                result.push_back(
                    {path, it->second.asset->format(), it->second.asset->memory_size(), it->second.asset.use_count()});
        }
        return result;
    }

  private:
    void evict_locked();       /**< evict() without locking (caller holds mutex_). */
    mutable std::mutex mutex_; /**< Guards all mutable state below. */
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
