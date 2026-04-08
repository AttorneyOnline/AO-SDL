/**
 * @file ASNReputationManager.h
 * @brief Per-ASN abuse tracking with automatic escalation.
 *
 * Tracks abuse events per Autonomous System Number (ASN) and automatically
 * escalates through status levels: NORMAL → WATCHED → RATE_LIMITED → BLOCKED.
 *
 * Aggressive defaults: 2/3/5 unique abusive IPs within 1 hour trigger
 * escalation. Large ISP ASNs can be whitelisted with higher thresholds.
 *
 * Thread safety: all public methods are mutex-protected. Persistence uses
 * the same async writer pattern as BanManager.
 */
#pragma once

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct ASNReputationEntry {
    uint32_t asn = 0;
    std::string as_org;

    int total_abuse_events = 0;

    /// Distinct IPs that have triggered abuse from this ASN (within window).
    /// NOTE: This set grows monotonically within a window — individual IPs
    /// are not removed when their events age out; only when the entire event
    /// list is empty (after prune_window). This means "unique IPs in window"
    /// is slightly overestimated: an IP that triggered once 55 min ago still
    /// counts if any other event keeps the window alive. This is intentional —
    /// for botnet detection, an IP that participated in an attack should count
    /// for the full window even if it disconnected.
    std::unordered_set<std::string> abusive_ips_in_window;

    /// Timestamps of recent abuse events (for sliding window).
    /// Stored as unix seconds. Oldest entries pruned on access.
    std::vector<int64_t> recent_event_times;

    enum class Status { NORMAL, WATCHED, RATE_LIMITED, BLOCKED };
    Status status = Status::NORMAL;
    int64_t status_changed_at = 0; ///< Unix seconds
    int64_t block_expires_at = 0;  ///< 0 = permanent, >0 = unix seconds
    std::string block_reason;
};

/// Configuration for ASN reputation.
struct ASNReputationConfig {
    bool enabled = true;
    int watch_threshold = 2;                 ///< Unique abusive IPs → WATCHED
    int rate_limit_threshold = 3;            ///< Unique abusive IPs → RATE_LIMITED
    int block_threshold = 5;                 ///< Unique abusive IPs → BLOCKED
    int window_minutes = 60;                 ///< Sliding window for threshold counting
    std::string auto_block_duration = "24h"; ///< Duration string for auto-blocks
    std::vector<uint32_t> whitelist_asns;    ///< ASNs with higher thresholds
    int whitelist_multiplier = 5;            ///< Threshold multiplier for whitelisted ASNs
};

/// Callback invoked when an ASN's status changes.
using ASNStatusCallback = std::function<void(uint32_t asn, ASNReputationEntry::Status old_status,
                                             ASNReputationEntry::Status new_status, const std::string& reason)>;

class ASNReputationManager {
  public:
    ASNReputationManager();
    ~ASNReputationManager();

    ASNReputationManager(const ASNReputationManager&) = delete;
    ASNReputationManager& operator=(const ASNReputationManager&) = delete;

    /// Apply configuration. Safe to call at any time.
    void configure(const ASNReputationConfig& config);

    /// Set a callback for status changes (e.g. to trigger firewall blocks).
    void set_status_callback(ASNStatusCallback callback);

    /// Report an abuse event from a specific IP within an ASN.
    /// This is the primary entry point — called by SpamDetector, BanManager, etc.
    void report_abuse(uint32_t asn, const std::string& ip, const std::string& as_org, const std::string& reason);

    /// Check if an ASN is blocked. Returns the entry if blocked.
    std::optional<ASNReputationEntry> check_blocked(uint32_t asn) const;

    /// Get the current status of an ASN.
    std::optional<ASNReputationEntry> get_status(uint32_t asn) const;

    /// Manually block an ASN.
    void block_asn(uint32_t asn, const std::string& as_org, const std::string& reason, int64_t duration_sec);

    /// Manually unblock an ASN.
    bool unblock_asn(uint32_t asn);

    /// List all ASNs with non-NORMAL status.
    std::vector<ASNReputationEntry> list_flagged() const;

    /// Prune expired blocks, old events outside the window. Returns count of ASNs reset.
    size_t sweep();

    /// Load from JSON file.
    void load(const std::string& path);

    /// Enqueue async save.
    void save_async(const std::string& path);

    /// Synchronous save.
    void save_sync(const std::string& path) const;

    /// Number of tracked ASNs.
    size_t count() const;

  private:
    ASNReputationConfig config_;
    ASNStatusCallback status_callback_;

    mutable std::mutex mutex_;
    std::unordered_map<uint32_t, ASNReputationEntry> asns_;

    /// Check if an ASN is whitelisted (higher thresholds apply).
    bool is_whitelisted(uint32_t asn) const;

    /// Get effective threshold for an ASN (accounts for whitelist multiplier).
    int effective_threshold(int base_threshold, uint32_t asn) const;

    /// Prune old events outside the sliding window for an entry.
    void prune_window(ASNReputationEntry& entry) const;

    /// Evaluate thresholds and potentially escalate status.
    void evaluate_escalation(ASNReputationEntry& entry);

    int64_t now_unix() const;

    // --- Persistence ---
    // writer_mutex_ and writer_cv_ must be declared BEFORE writer_thread_
    // because members initialize in declaration order, and the jthread
    // starts immediately — it must not touch uninitialized synchronization primitives.
    std::mutex writer_mutex_;
    std::condition_variable writer_cv_;
    bool writer_pending_ = false;
    std::string writer_path_;
    std::unordered_map<uint32_t, ASNReputationEntry> writer_snapshot_;
    std::jthread writer_thread_;

    void writer_loop(std::stop_token stop);
    std::unordered_map<uint32_t, ASNReputationEntry> snapshot() const;
    static void write_to_disk(const std::unordered_map<uint32_t, ASNReputationEntry>& asns, const std::string& path);
};

/// Convert status enum to string.
const char* asn_status_to_string(ASNReputationEntry::Status s);
