/**
 * @file Mount.h
 * @brief Abstract base class for mount backends (directories, 7z archives, etc.).
 *
 * A Mount represents a single source of asset data (a directory on disk, a 7z
 * archive, etc.). MountManager holds a collection of these and queries them
 * in priority order.
 */
#pragma once

#include <filesystem>

/**
 * @brief Abstract base for virtual filesystem mount backends.
 *
 * Subclasses implement directory mounts, 7z archive mounts, or other
 * storage backends. Each mount can be loaded, queried for file existence,
 * and asked to return raw file bytes.
 */
class Mount {
  public:
    /**
     * @brief Construct a Mount targeting a specific filesystem path.
     * @param target_path Path to the directory or archive this mount represents.
     */
    Mount(const std::filesystem::path& target_path) : path{target_path} {};

    /**
     * @brief Get the filesystem path this mount was constructed with.
     * @return The target path (directory or archive).
     */
    std::filesystem::path get_path() const {
        return path;
    };

    /**
     * @brief Load and index the mount's contents.
     *
     * Called once after construction. Implementations should scan the
     * directory or read the archive index so that seek_file() and
     * fetch_data() work correctly.
     */
    virtual void load() = 0;

    /**
     * @brief Check whether a file exists in this mount.
     * @param path Virtual (relative) path to look up.
     * @return True if the file exists in this mount.
     */
    virtual bool seek_file(const std::string& path) const = 0;

    /**
     * @brief Read a file's contents from this mount.
     * @param path Virtual (relative) path to the file.
     * @return The raw file bytes.
     * @pre seek_file(path) returns true.
     */
    virtual std::vector<uint8_t> fetch_data(const std::string& path) = 0;

  protected:
    const std::filesystem::path path; /**< Filesystem path to the backing directory or archive. */

    /**
     * @brief Load a previously saved index cache from disk (implementation-defined).
     */
    virtual void load_cache() = 0;

    /**
     * @brief Save the current index to a cache file on disk (implementation-defined).
     */
    virtual void save_cache() = 0;
};
