#pragma once

#include "asset/Mount.h"
#include "net/HttpPool.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

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

    void load() override;
    bool seek_file(const std::string& path) const override;
    std::vector<uint8_t> fetch_data(const std::string& path) override;

    /// Trigger an async download for the given path if not already
    /// cached, pending, or previously failed (404).
    void request(const std::string& path);

    /// Number of downloads currently in-flight.
    int pending_count() const;

  protected:
    void load_cache() override {}
    void save_cache() override {}

  private:
    std::string base_url_;  // e.g. "https://server.com/assets"
    std::string host_;      // e.g. "https://server.com"
    std::string path_prefix_; // e.g. "/assets"
    HttpPool& pool_;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<uint8_t>> cache_;
    std::unordered_set<std::string> pending_;
    std::unordered_set<std::string> failed_;
};
