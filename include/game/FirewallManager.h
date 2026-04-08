/**
 * @file FirewallManager.h
 * @brief Kernel-level IP blocking via a setcap helper binary.
 *
 * Manages iptables/nftables rules through a small helper binary that has
 * CAP_NET_ADMIN capability set. The server process itself runs unprivileged.
 *
 * If the helper binary is not configured or not found, the manager degrades
 * gracefully — is_enabled() returns false and all operations are no-ops.
 *
 * Thread safety: all public methods are mutex-protected. Helper invocations
 * run on a background thread to avoid blocking the caller.
 */
#pragma once

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

/// Duration value meaning "permanent" (no expiry). Used by block_ip/block_range/block_asn.
inline constexpr int64_t DURATION_PERMANENT = -2;

struct FirewallRule {
    std::string target; ///< IP address or CIDR range
    std::string reason;
    int64_t installed_at = 0; ///< Unix seconds
    int64_t expires_at = 0;   ///< 0 = permanent, >0 = unix seconds
};

/// Configuration for the firewall manager.
struct FirewallConfig {
    bool enabled = false;    ///< Disabled by default (requires setup)
    std::string helper_path; ///< Path to kagami-fw-helper binary
    bool cleanup_on_shutdown = true;
};

class FirewallManager {
  public:
    FirewallManager();
    ~FirewallManager();

    FirewallManager(const FirewallManager&) = delete;
    FirewallManager& operator=(const FirewallManager&) = delete;

    /// Apply configuration. Checks if helper binary exists and is executable.
    void configure(const FirewallConfig& config);

    /// Returns true if the firewall manager is configured and operational.
    bool is_enabled() const;

    /// Block an individual IP address.
    bool block_ip(const std::string& ip, const std::string& reason, int64_t duration_sec = DURATION_PERMANENT);

    /// Unblock an individual IP address.
    bool unblock_ip(const std::string& ip);

    /// Block a CIDR range (e.g. "200.174.198.0/24").
    bool block_range(const std::string& cidr, const std::string& reason, int64_t duration_sec = DURATION_PERMANENT);

    /// Unblock a CIDR range.
    bool unblock_range(const std::string& cidr);

    /// Remove expired rules. Returns count removed.
    size_t sweep_expired();

    /// List all active rules.
    std::vector<FirewallRule> list_rules() const;

    /// Remove all rules installed by this manager.
    void flush();

    /// Load rules from crash recovery file.
    void load(const std::string& path);

    /// Save current rules for crash recovery.
    void save_sync(const std::string& path) const;

  private:
    FirewallConfig config_;
    bool enabled_ = false; ///< Verified: helper exists and is executable

    mutable std::mutex mutex_;
    std::unordered_map<std::string, FirewallRule> rules_; ///< Keyed by target

    // --- Background execution queue ---
    struct HelperCommand {
        std::string action; ///< "add", "remove", "add-range", "remove-range", "flush"
        std::string target; ///< IP or CIDR (empty for "flush")
    };
    std::mutex exec_mutex_;
    std::condition_variable exec_cv_;
    std::deque<HelperCommand> exec_queue_;
    std::jthread exec_thread_;

    void exec_loop(std::stop_token stop);

    /// Execute the helper binary synchronously. Returns true on success.
    /// Must be called from the exec thread only.
    bool exec_helper(const std::string& action, const std::string& target);

    /// Validate an IP address format (basic regex check).
    static bool is_valid_ip(const std::string& ip);

    /// Validate a CIDR range format.
    static bool is_valid_cidr(const std::string& cidr);

    /// Enqueue a helper command for background execution.
    void enqueue(HelperCommand cmd);

    int64_t now_unix() const;
};
