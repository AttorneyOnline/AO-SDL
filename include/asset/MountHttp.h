#pragma once

#include "asset/Mount.h"
#include "net/HttpPool.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/// Mount backend that fetches files over HTTP on demand.
///
/// Files are downloaded lazily: call request() to trigger a background
/// download via HttpPool. Once complete, the data is available through
/// the normal seek_file()/fetch_data() interface.
///
/// Thread-safe: the internal cache is protected by a mutex.
class MountHttp : public Mount {
  public:
    /// @param base_url  Scheme + host + path prefix, e.g. "https://server.com/assets/"
    /// @param pool      Reference to the HTTP thread pool (must outlive this mount).
    MountHttp(const std::string& base_url, HttpPool& pool);
    ~MountHttp() override;

    void load() override;
    bool seek_file(const std::string& path) const override;
    std::vector<uint8_t> fetch_data(const std::string& path) override;
    std::vector<uint8_t> try_fetch(const std::string& path) override;

    /// Trigger an async download for the given path if not already
    /// cached, pending, or previously failed (404).
    void request(const std::string& path, HttpPriority priority = HttpPriority::NORMAL);

    /// Streaming download. Calls on_chunk with each received chunk.
    /// Returns true if the download succeeded (200), false on error/404.
    /// Blocks until the full download completes or fails.
    bool fetch_streaming(const std::string& path, std::function<bool(const uint8_t*, size_t)> on_chunk);

    /// Number of downloads currently in-flight.
    int pending_count() const;
    /// Number of files cached in memory.
    int cached_count() const;
    /// Number of paths that returned 404 or error.
    int failed_count() const;
    /// Check if a specific path has been tried and failed (404/error).
    bool has_failed(const std::string& path) const;
    /// Total bytes stored in the raw download cache.
    size_t cached_bytes() const;

    /// Remove a file from the raw byte cache (called after decode/cache).
    void release(const std::string& path);

    /// Release all raw bytes from the cache. Files that have been decoded
    /// into AssetCache don't need the raw bytes anymore. Files that haven't
    /// been decoded will be re-downloaded on next access.
    void release_all();

    struct CacheEntry {
        std::string path;
        size_t bytes;
    };
    /// Snapshot of the raw byte cache for debug display.
    std::vector<CacheEntry> cache_snapshot() const;

    /// Asset type categories for extension lookup.
    enum class AssetType { CHARICON, EMOTE, EMOTIONS, BACKGROUND };

    /// Get the server-advertised extensions for an asset type.
    /// Returns the extensions from extensions.json (with leading dots stripped).
    /// Falls back to defaults if extensions.json hasn't been fetched yet.
    std::vector<std::string> extensions_for(AssetType type) const;

    /// Whether extensions.json has been loaded.
    bool has_extensions() const;

    /// Monotonically increasing counter bumped on each successful HTTP cache insertion.
    /// Consumers can compare against a stored value to detect new data arrivals.
    uint32_t cache_generation() const {
        return cache_generation_.load(std::memory_order_relaxed);
    }

    /// Access the underlying HttpPool (for drop_below, pending count, etc.).
    HttpPool& pool() {
        return pool_;
    }

  protected:
    void load_cache() override {
    }
    void save_cache() override {
    }

  private:
    std::string base_url_;    // e.g. "https://server.com/assets"
    std::string host_;        // e.g. "https://server.com"
    std::string path_prefix_; // e.g. "/assets"
    HttpPool& pool_;

    void fetch_extensions();

    // Shared flag checked by async callbacks to detect destruction.
    // Callbacks capture this by value; the destructor sets it to false.
    // Safe without a mutex because poll() and ~MountHttp run on the same thread.
    std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);

    std::atomic<uint32_t> cache_generation_{0};

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<uint8_t>> cache_;
    std::unordered_set<std::string> pending_;
    std::unordered_set<std::string> failed_;                  // Permanent (404)
    std::unordered_map<std::string, int> transient_failures_; // Retryable (SSL/timeout), value = attempt count
    static constexpr int max_retries_ = 3;

    // extensions.json data (guarded by mutex_)
    bool extensions_loaded_ = false;
    std::vector<std::string> charicon_exts_;
    std::vector<std::string> emote_exts_;
    std::vector<std::string> emotions_exts_;
    std::vector<std::string> background_exts_;
};
