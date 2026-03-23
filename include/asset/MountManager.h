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
#include <functional>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <vector>

#include "Mount.h"

class MountHttp;

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
     * @brief Append a mount to the end of the mount list (lowest priority).
     *
     * @note Acquires an exclusive lock on the internal shared_mutex.
     */
    void add_mount(std::unique_ptr<Mount> mount);

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

    /// Get the server-advertised extensions for an asset type from the HTTP mount.
    /// Returns empty if no HTTP mount or extensions.json not loaded yet.
    std::vector<std::string> http_extensions(int asset_type) const;

    /**
     * @brief Trigger async HTTP downloads for a file not found in local mounts.
     *
     * If the file exists in a local (non-HTTP) mount, does nothing.
     * Otherwise, calls request() on all HTTP mounts so the file
     * will be available on a future fetch_data() call.
     *
     * @param relative_path The virtual path to prefetch.
     */
    void prefetch(const std::string& relative_path, int priority = 1);

    /// Stream a file from the first HTTP mount that has it.
    /// Calls on_chunk with each received chunk. Returns true on success.
    /// Blocks until download completes. For use from background threads.
    bool fetch_streaming(const std::string& relative_path, std::function<bool(const uint8_t*, size_t)> on_chunk);

    /// Stream from a direct HTTP/HTTPS URL (not relative to any mount).
    /// Blocks until download completes. For use from background threads.
    bool fetch_streaming_url(const std::string& url, std::function<bool(const uint8_t*, size_t)> on_chunk);

    /// Release raw bytes from HTTP mount caches after the data has been
    /// decoded and stored in AssetCache. Frees the duplicate memory.
    void release_http(const std::string& relative_path);

    /// Release all raw bytes from HTTP mount caches.
    void release_all_http();

    /// Drop all queued HTTP requests below the given priority level.
    void drop_http_below(int priority);

    /// Get HTTP mount stats (pending downloads, cached files, failed files).
    struct HttpStats {
        int pending = 0;
        int cached = 0;
        int failed = 0;
        int pool_pending = 0;
        size_t cached_bytes = 0;
    };
    HttpStats http_stats() const;

    struct HttpCacheEntry {
        std::string path;
        size_t bytes;
    };
    std::vector<HttpCacheEntry> http_cache_snapshot() const;

  private:
    mutable std::shared_mutex lock; /**< Protects loaded_mounts. Shared for reads, exclusive for writes. */
    std::vector<std::unique_ptr<Mount>> loaded_mounts; /**< Ordered list of active mount backends. */
};
