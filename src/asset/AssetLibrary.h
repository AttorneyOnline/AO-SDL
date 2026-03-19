#pragma once

#include "Asset.h"
#include "AssetCache.h"
#include "ImageAsset.h"

#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using IniDocument = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

// Master asset manager. Owns the cache and is the single point of entry for
// loading and retrieving assets. All I/O is delegated to the MountManager.
//
// Callers receive shared_ptr<T> handles. Holding a handle pins the asset in
// memory — the cache will not evict it until all handles are released.
//
// Extension probing (webp/apng/gif/png for images, opus/ogg/mp3/wav for audio)
// lives here — it is a format-level policy, not a game-level concern.
class MountManager;

class AssetLibrary {
  public:
    explicit AssetLibrary(MountManager& mounts, size_t cache_max_bytes);

    // Retrieve a decoded image asset. The path is a virtual path without extension.
    // Probes: webp → apng → gif → png.
    std::shared_ptr<ImageAsset> image(const std::string& path);

    // Retrieve a raw audio asset. The path is a virtual path without extension.
    // Probes: opus → ogg → mp3 → wav.
    std::shared_ptr<Asset> audio(const std::string& path);

    // Retrieve and parse an INI config file at the given virtual path (with extension).
    std::optional<IniDocument> config(const std::string& path);

    // Retrieve a shader source file (with extension).
    std::shared_ptr<Asset> shader(const std::string& path);

    // Retrieve a font file (with extension).
    std::shared_ptr<Asset> font(const std::string& path);

    // Fetch raw bytes at an exact virtual path. Bypasses the asset cache.
    // Use for one-off reads or assets with non-standard formats.
    std::optional<std::vector<uint8_t>> raw(const std::string& path);

    // List the contents of a virtual directory across all mounts.
    std::vector<std::string> list(const std::string& directory);

    // Evict all cached assets that are not currently held by any caller.
    // Useful after a scene transition to free memory from the previous scene.
    void evict_unused();

    const AssetCache& cache() const { return m_cache; }

  private:
    // Probe a path against a prioritised list of extensions.
    // Returns the resolved path (with extension) and raw bytes, or nullopt.
    std::optional<std::pair<std::string, std::vector<uint8_t>>>
    probe(const std::string& path, const std::vector<std::string>& extensions);

    MountManager& m_mounts;
    AssetCache m_cache;
};
