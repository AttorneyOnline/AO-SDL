#include "MediaManager.h"

#include "MountManager.h"

static constexpr size_t DEFAULT_CACHE_BYTES = 256 * 1024 * 1024; // 256 MB

MediaManager::MediaManager() : m_mounts(std::make_unique<MountManager>()) {
}

MediaManager& MediaManager::instance() {
    static MediaManager instance;
    return instance;
}

void MediaManager::init(const std::filesystem::path& base_path) {
    m_mounts->load_mounts({base_path});
    m_library = std::make_unique<AssetLibrary>(*m_mounts, DEFAULT_CACHE_BYTES);
}

AssetLibrary& MediaManager::assets() {
    return *m_library;
}
