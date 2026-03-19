#include "AssetCache.h"

AssetCache::AssetCache(size_t max_bytes) : m_max_bytes(max_bytes) {}

std::shared_ptr<Asset> AssetCache::get(const std::string& path) {
    auto it = m_entries.find(path);
    if (it == m_entries.end()) return nullptr;

    // Promote to front of LRU list.
    m_lru.splice(m_lru.begin(), m_lru, it->second.lru_pos);

    return it->second.asset;
}

void AssetCache::insert(std::shared_ptr<Asset> asset) {
    const std::string& path = asset->path();

    // If already cached, remove the old entry's accounting first.
    auto it = m_entries.find(path);
    if (it != m_entries.end()) {
        m_used_bytes -= it->second.asset->memory_size();
        m_lru.erase(it->second.lru_pos);
        m_entries.erase(it);
    }

    m_used_bytes += asset->memory_size();
    m_lru.push_front(path);
    m_entries[path] = {asset, m_lru.begin()};

    evict_to_limit();
}

void AssetCache::evict_unused() {
    for (auto it = m_entries.begin(); it != m_entries.end();) {
        if (it->second.asset.use_count() == 1) {
            m_used_bytes -= it->second.asset->memory_size();
            m_lru.erase(it->second.lru_pos);
            it = m_entries.erase(it);
        }
        else {
            ++it;
        }
    }
}

void AssetCache::evict_to_limit() {
    // Walk LRU from back (least recently used), evicting only unpinned entries.
    auto it = m_lru.end();
    while (m_used_bytes > m_max_bytes && it != m_lru.begin()) {
        --it;
        auto entry_it = m_entries.find(*it);
        if (entry_it != m_entries.end() && entry_it->second.asset.use_count() == 1) {
            m_used_bytes -= entry_it->second.asset->memory_size();
            it = m_lru.erase(it);
            m_entries.erase(entry_it);
        }
    }
}
