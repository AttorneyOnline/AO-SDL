/**
 * @file MountManager.h
 * @brief Manages virtual filesystem mounts (directories and archives).
 *
 * MountManager aggregates multiple Mount backends and provides a unified
 * interface for fetching file data by virtual path.
 *
 * @note Thread-safe: all public methods are protected by a std::shared_mutex.
 *       Reads (fetch_data) acquire a shared lock; writes (load_mounts) acquire
 *       an exclusive lock.
 */
#pragma once

#include <filesystem>
#include <shared_mutex>
#include <vector>

#include "Mount.h"

/**
 * @brief Aggregates Mount backends and provides unified virtual file access.
 *
 * Mounts are searched in order; the first mount that contains the requested
 * file wins.
 *
 * @note Thread-safe via std::shared_mutex. Concurrent reads are allowed;
 *       load_mounts() takes an exclusive lock.
 */
class MountManager {
  public:
    /** @brief Construct an empty MountManager with no mounts loaded. */
    MountManager();

    /**
     * @brief Load mount backends from a list of filesystem paths.
     *
     * Each path may be a directory or an archive. Replaces any previously
     * loaded mounts.
     *
     * @note Acquires an exclusive lock on the internal shared_mutex.
     *
     * @param target_mount_path Vector of filesystem paths to mount.
     */
    void load_mounts(const std::vector<std::filesystem::path>& target_mount_path);

    /**
     * @brief Fetch file data by virtual (relative) path from the first matching mount.
     *
     * Searches mounts in order and returns data from the first mount that
     * contains the file.
     *
     * @note Acquires a shared lock on the internal shared_mutex.
     *
     * @param relative_path The virtual path to look up (e.g. "characters/Phoenix/normal.png").
     * @return The file contents as a byte vector, or std::nullopt if not found in any mount.
     */
    std::optional<std::vector<uint8_t>> fetch_data(const std::string& relative_path);

  private:
    std::shared_mutex lock;                        /**< Protects loaded_mounts. Shared for reads, exclusive for writes. */
    std::vector<std::unique_ptr<Mount>> loaded_mounts; /**< Ordered list of active mount backends. */
};
