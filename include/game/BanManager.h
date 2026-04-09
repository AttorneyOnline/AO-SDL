/**
 * @file BanManager.h
 * @brief Protocol-agnostic ban storage backed by DatabaseManager (SQLite).
 *
 * Bans are keyed by IPID (SHA-256 of IP, first 8 hex chars). The manager
 * also supports lookup by HDID (hardware ID) for hardware bans.
 *
 * Architecture: in-memory cache (unordered_map) for O(1) ban checks on
 * every connection, with write-through to SQLite via DatabaseManager.
 * The in-memory map is authoritative for reads; the DB is authoritative
 * for persistence across restarts.
 *
 * Thread safety: all public methods are mutex-protected. DB writes are
 * fire-and-forget dispatches to the DatabaseManager worker thread.
 */
#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class DatabaseManager;
class FirewallManager;

struct BanEntry {
    int64_t id = 0; ///< Database row ID (0 = not yet persisted).
    std::string ipid;
    std::string hdid;
    std::string ip; ///< Raw IP address (for firewall blocking).
    std::string reason;
    std::string moderator; ///< Display name of the banning moderator.
    int64_t timestamp = 0; ///< Unix seconds when the ban was issued.
    int64_t duration = 0;  ///< Duration in seconds. -2 = permanent, 0 = invalidated.

    bool is_permanent() const {
        return duration == -2;
    }

    bool is_expired() const {
        if (is_permanent())
            return false;
        if (duration == 0)
            return true; // invalidated
        auto now = std::chrono::system_clock::now();
        auto expires = std::chrono::system_clock::from_time_t(timestamp) + std::chrono::seconds(duration);
        return now > expires;
    }
};

/// Parse a duration string into seconds. Returns -2 for "perma"/"permanent".
/// Supports: "perma", "30s", "5m", "1h", "2h30m", "1h30m15s".
/// Returns -1 on parse failure.
int64_t parse_ban_duration(const std::string& input);

/// Format a duration in seconds to a human-readable string.
/// -2 = "permanent", 0 = "invalidated", >0 = "Xh Ym Zs".
std::string format_ban_duration(int64_t duration);

class BanManager {
  public:
    BanManager() = default;
    ~BanManager() = default;

    BanManager(const BanManager&) = delete;
    BanManager& operator=(const BanManager&) = delete;

    /// Set the database backend. Call before load_from_db().
    /// If null, operates in-memory only (for tests).
    void set_db(DatabaseManager* db) {
        db_ = db;
    }

    /// Set the firewall backend. If set, bans will also block/unblock at the kernel level.
    void set_firewall(FirewallManager* fw) {
        fw_ = fw;
    }

    /// Load all active bans from the database into the in-memory cache.
    /// Blocks on startup. Call after set_db().
    void load_from_db();

    /// Add a ban. Overwrites any existing ban for the same IPID.
    /// Writes through to DB asynchronously.
    void add_ban(BanEntry entry);

    /// Remove (invalidate) a ban by IPID. Returns true if a ban was found.
    /// Soft-deletes in DB (sets duration=0) to preserve audit history.
    bool remove_ban(const std::string& ipid);

    /// Update a ban field ("reason" or "duration") by IPID.
    /// Returns true if the ban was found and updated.
    bool update_ban(const std::string& ipid, const std::string& field, const std::string& value);

    /// Check if banned and return the ban entry in a single lock acquisition.
    /// Returns std::nullopt if not banned.
    std::optional<BanEntry> check_ban(const std::string& ipid, const std::string& hdid) const;

    /// Find a ban by IPID. Returns a copy (safe to use outside the lock).
    std::optional<BanEntry> find_ban(const std::string& ipid) const;

    /// Find a ban by HDID across all entries. Returns a copy.
    std::optional<BanEntry> find_ban_by_hdid(const std::string& hdid) const;

    /// Search bans by substring match (case-insensitive) against IPID, HDID,
    /// reason, and moderator. Returns up to `limit` matching entries.
    std::vector<BanEntry> search_bans(const std::string& query, int limit = 10) const;

    /// Return all bans sorted by timestamp descending, up to `limit`.
    std::vector<BanEntry> list_bans(int limit = 20) const;

    /// Number of active (non-expired) bans.
    size_t count() const;

  private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, BanEntry> bans_; ///< Keyed by IPID.
    DatabaseManager* db_ = nullptr;
    FirewallManager* fw_ = nullptr;
};
