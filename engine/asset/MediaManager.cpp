#include "asset/MediaManager.h"

#include "asset/MountManager.h"

static constexpr size_t DEFAULT_CACHE_BYTES = 256 * 1024 * 1024; // 256 MB

MediaManager::MediaManager()
    : mounts(std::make_unique<MountManager>()),
      library(std::make_unique<AssetLibrary>(*mounts, DEFAULT_CACHE_BYTES)) {
}

MediaManager& MediaManager::instance() {
    static MediaManager instance;
    return instance;
}

void MediaManager::init(const std::filesystem::path& base_path) {
    mounts->load_mounts({base_path});
    library = std::make_unique<AssetLibrary>(*mounts, DEFAULT_CACHE_BYTES);
}

AssetLibrary& MediaManager::assets() {
    return *library;
}

void MediaManager::shutdown() {
    library.reset();
    mounts.reset();
}
