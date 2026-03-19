#pragma once

#include "AssetLibrary.h"

#include <filesystem>
#include <memory>

class MountManager;

class MediaManager {
  public:
    static MediaManager& instance();

    // Load a base directory (and optional additional mounts) as the asset source.
    // Must be called before any assets() calls. Can be called again to reload.
    void init(const std::filesystem::path& base_path);

    AssetLibrary& assets();

  private:
    MediaManager();

    MediaManager(MediaManager&) = delete;
    void operator=(MediaManager const&) = delete;

    std::unique_ptr<MountManager> m_mounts;
    std::unique_ptr<AssetLibrary> m_library;
};
