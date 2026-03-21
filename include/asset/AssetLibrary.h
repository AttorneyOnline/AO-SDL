/**
 * @file AssetLibrary.h
 * @brief Single entry point for loading assets with extension probing.
 *
 * AssetLibrary is the master asset manager. It owns an AssetCache and is the
 * sole entry point for loading and retrieving assets. All filesystem I/O is
 * delegated to MountManager.
 */
#pragma once

#include "Asset.h"
#include "AssetCache.h"
#include "ImageAsset.h"
#include "ShaderAsset.h"

#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

/** @brief Parsed INI document: section name -> (key -> value). */
using IniDocument = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

class MountManager;

/**
 * @brief Master asset manager providing cached, format-probed asset loading.
 *
 * Callers receive shared_ptr<T> handles. Holding a handle pins the asset in
 * memory -- the cache will not evict it until all handles are released.
 *
 * Extension probing (webp/apng/gif/png for images, opus/ogg/mp3/wav for audio)
 * lives here as a format-level policy rather than a game-level concern.
 */
class AssetLibrary {
  public:
    /**
     * @brief Construct an AssetLibrary backed by a MountManager and a cache budget.
     * @param mounts Reference to the MountManager for filesystem access.
     *               Must outlive this AssetLibrary.
     * @param cache_max_bytes Soft memory limit for the internal AssetCache.
     */
    explicit AssetLibrary(MountManager& mounts, size_t cache_max_bytes);

    /**
     * @brief Load a decoded image asset by virtual path (no extension).
     *
     * Probes in order: webp, apng, gif, png. Returns a cached result if
     * available, otherwise decodes from disk and caches.
     *
     * @param path Virtual path without extension (e.g. "characters/Phoenix/normal").
     * @return Shared pointer to the decoded ImageAsset, or nullptr if not found.
     */
    std::shared_ptr<ImageAsset> image(const std::string& path);

    /**
     * @brief Load a raw audio asset by virtual path (no extension).
     *
     * Probes in order: opus, ogg, mp3, wav.
     *
     * @param path Virtual path without extension.
     * @return Shared pointer to the audio Asset, or nullptr if not found.
     */
    std::shared_ptr<Asset> audio(const std::string& path);

    /**
     * @brief Load and parse an INI config file at the given virtual path.
     * @param path Virtual path including the file extension.
     * @return The parsed IniDocument, or std::nullopt if the file was not found.
     */
    std::optional<IniDocument> config(const std::string& path);

    /// Set the GPU backend name (e.g. "OpenGL", "Metal"). Called once at
    /// startup so shader() knows which subdirectory to probe.
    void set_shader_backend(const std::string& backend) { shader_backend_ = backend; }

    /**
     * @brief Load a shader pair (vertex + fragment) by virtual path.
     *
     * Probes backend-specific subdirectories based on the configured backend:
     *   path/glsl/vertex.{glsl,vert} + path/glsl/fragment.{glsl,frag}
     *   path/metal/vertex.metal      + path/metal/fragment.metal
     *
     * @param path Virtual path prefix (e.g. "shaders/screentint").
     * @return Shared pointer to the ShaderAsset, or nullptr if not found.
     */
    std::shared_ptr<ShaderAsset> shader(const std::string& path);

    /**
     * @brief Load a font file by virtual path (with extension).
     * @param path Virtual path including the file extension.
     * @return Shared pointer to the font Asset, or nullptr if not found.
     */
    std::shared_ptr<Asset> font(const std::string& path);

    /**
     * @brief Fetch raw bytes at an exact virtual path. Bypasses the asset cache.
     *
     * Use for one-off reads or assets with non-standard formats.
     *
     * @param path Exact virtual path including extension.
     * @return The raw file bytes, or std::nullopt if not found.
     */
    std::optional<std::vector<uint8_t>> raw(const std::string& path);

    /**
     * @brief List the contents of a virtual directory across all mounts.
     * @param directory The virtual directory path to list.
     * @return A vector of entry names found in the directory.
     */
    std::vector<std::string> list(const std::string& directory);

    /**
     * @brief Register a manually-created asset in the cache.
     *
     * Use for procedurally generated assets (e.g. GPU-rendered text).
     * The asset's path() is used as the cache key.
     */
    void register_asset(std::shared_ptr<Asset> asset);

    /**
     * @brief Look up a cached asset by path (promotes LRU).
     * @return Shared pointer to the asset, or nullptr if not cached.
     */
    std::shared_ptr<Asset> get_cached(const std::string& path) {
        return cache_.get(path);
    }

    /**
     * @brief Evict LRU entries until the cache is within its memory budget.
     *
     * Only evicts entries not currently held by any caller. Safe to call
     * periodically — does nothing if the cache is already under budget.
     */
    void evict();

    /**
     * @brief Get a const reference to the internal AssetCache.
     * @return The underlying AssetCache.
     */
    const AssetCache& cache() const {
        return cache_;
    }

  private:
    /**
     * @brief Probe a path against a prioritized list of extensions.
     * @param path Virtual path without extension.
     * @param extensions List of extensions to try, in priority order.
     * @return A pair of (resolved path with extension, raw bytes), or std::nullopt.
     */
    std::optional<std::pair<std::string, std::vector<uint8_t>>> probe(const std::string& path,
                                                                      const std::vector<std::string>& extensions);

    MountManager& mounts;          /**< Filesystem mount manager. */
    AssetCache cache_;             /**< Internal LRU asset cache. */
    std::string shader_backend_;   /**< GPU backend name for shader path probing. */
};
