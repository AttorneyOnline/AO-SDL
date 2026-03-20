/**
 * @file MediaManager.h
 * @brief Singleton owner of MountManager and AssetLibrary.
 *
 * MediaManager is the top-level entry point for the asset system. It owns the
 * MountManager (virtual filesystem) and AssetLibrary (cached asset loading).
 */
#pragma once

#include "AssetLibrary.h"

#include <filesystem>
#include <memory>

class MountManager;

/**
 * @brief Singleton that owns the asset system (MountManager + AssetLibrary).
 *
 * Provides global access to asset loading facilities. init() must be called
 * with a base content path before any calls to assets().
 *
 * @note This is a singleton -- the constructor is private and the copy/assign
 *       operators are deleted. Access via instance().
 * @note init() may be called more than once to reload mounts from a new path.
 */
class MediaManager {
  public:
    /**
     * @brief Get the singleton MediaManager instance.
     * @return Reference to the global MediaManager.
     */
    static MediaManager& instance();

    /**
     * @brief Initialize (or reinitialize) the asset system with a base content directory.
     *
     * Loads the base directory and any additional mounts discovered within it.
     * Must be called at least once before assets() is used.
     * Can be called again to reload from a different path.
     *
     * @param base_path Filesystem path to the root content directory.
     */
    void init(const std::filesystem::path& base_path);

    /**
     * @brief Get the AssetLibrary for loading assets.
     *
     * @pre init() must have been called at least once.
     *
     * @return Reference to the AssetLibrary.
     */
    AssetLibrary& assets();

  private:
    /** @brief Private constructor (singleton pattern). */
    MediaManager();

    MediaManager(MediaManager&) = delete;
    void operator=(MediaManager const&) = delete;

    std::unique_ptr<MountManager> mounts;  /**< Virtual filesystem mount manager. */
    std::unique_ptr<AssetLibrary> library; /**< Cached asset loader. */
};
