/**
 * @file BanManager.h
 * @brief Protocol-agnostic ban storage with async JSON persistence.
 *
 * Bans are keyed by IPID (SHA-256 of IP, first 8 hex chars). The manager
 * also supports lookup by HDID (hardware ID) for hardware bans.
 *
 * Thread safety: all public methods are mutex-protected. save_async()
 * enqueues a write to a background writer thread that is joined on
 * destruction, ensuring in-flight writes complete before shutdown.
 */
#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

/// Default ban file path. Used by commands and main.cpp.
inline constexpr const char* DEFAULT_BAN_FILE = "bans.json";

struct BanEntry {
    std::string ipid;
    std::string hdid;
    std::string reason;
    std::string moderator; ///< Display name of the banning moderator.
    int64_t timestamp = 0; ///< Unix seconds when the ban was issued.
    int64_t duration = 0;  ///< Duration in seconds. -2 = permanent.

    bool is_permanent() const {
        return duration == -2;
    }

    bool is_expired() const {
        if (is_permanent())
            return false;
        auto now = std::chrono::system_clock::now();
        auto expires = std::chrono::system_clock::from_time_t(timestamp) + std::chrono::seconds(duration);
        return now > expires;
    }
};

/// Parse a duration string into seconds. Returns -2 for "perma"/"permanent".
/// Supports: "perma", "30s", "5m", "1h", "2h30m", "1h30m15s".
/// Returns -1 on parse failure.
int64_t parse_ban_duration(const std::string& input);

class BanManager {
  public:
    BanManager();
    ~BanManager();

    BanManager(const BanManager&) = delete;
    BanManager& operator=(const BanManager&) = delete;

    /// Add a ban. Overwrites any existing ban for the same IPID.
    void add_ban(BanEntry entry);

    /// Remove a ban by IPID. Returns true if a ban was removed.
    bool remove_ban(const std::string& ipid);

    /// Check if an IPID or HDID is banned (skipping expired entries).
    bool is_banned(const std::string& ipid, const std::string& hdid) const;

    /// Find a ban by IPID. Returns a copy (safe to use outside the lock).
    std::optional<BanEntry> find_ban(const std::string& ipid) const;

    /// Find a ban by HDID across all entries. Returns a copy.
    std::optional<BanEntry> find_ban_by_hdid(const std::string& hdid) const;

    /// Get the ban reason for a given IPID/HDID, or "Banned" if not found.
    /// Convenience method that does the lookup and copies the reason under the lock.
    std::string get_ban_reason(const std::string& ipid, const std::string& hdid) const;

    /// Load bans from a JSON file. Skips expired entries.
    void load(const std::string& path);

    /// Enqueue an async save. The background writer thread will write
    /// a snapshot of the ban map to disk. Joining the writer on
    /// destruction guarantees in-flight writes complete before shutdown.
    void save_async(const std::string& path);

    /// Synchronous save. Blocks until the file is written.
    void save_sync(const std::string& path) const;

    /// Number of active (non-expired) bans.
    size_t count() const;

  private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, BanEntry> bans_; ///< Keyed by IPID.

    /// Snapshot the ban map under the lock (for save operations).
    std::unordered_map<std::string, BanEntry> snapshot() const;

    /// Write a ban map snapshot to disk.
    static void write_to_disk(const std::unordered_map<std::string, BanEntry>& bans, const std::string& path);

    // Background writer thread
    std::jthread writer_thread_;
    std::mutex writer_mutex_;
    std::condition_variable writer_cv_;
    bool writer_pending_ = false;
    std::string writer_path_;
    std::unordered_map<std::string, BanEntry> writer_snapshot_;

    void writer_loop(std::stop_token stop);
};
