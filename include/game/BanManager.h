/**
 * @file BanManager.h
 * @brief Protocol-agnostic ban storage with async JSON persistence.
 *
 * Bans are keyed by IPID (SHA-256 of IP, first 8 hex chars). The manager
 * also supports lookup by HDID (hardware ID) for hardware bans.
 *
 * Thread safety: all public methods are mutex-protected. save_async()
 * copies the ban map under the mutex and writes on a detached thread.
 */
#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

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
    /// Add a ban. Overwrites any existing ban for the same IPID.
    void add_ban(BanEntry entry);

    /// Remove a ban by IPID. Returns true if a ban was removed.
    bool remove_ban(const std::string& ipid);

    /// Check if an IPID or HDID is banned (skipping expired entries).
    bool is_banned(const std::string& ipid, const std::string& hdid) const;

    /// Find a ban by IPID, or nullptr if not found/expired.
    const BanEntry* find_ban(const std::string& ipid) const;

    /// Find a ban by HDID across all entries, or nullptr if not found/expired.
    const BanEntry* find_ban_by_hdid(const std::string& hdid) const;

    /// Load bans from a JSON file. Skips expired entries.
    void load(const std::string& path);

    /// Async save: copies the ban map under mutex, writes JSON on a detached thread.
    void save_async(const std::string& path) const;

    /// Number of active (non-expired) bans.
    size_t count() const;

  private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, BanEntry> bans_; ///< Keyed by IPID.
};
