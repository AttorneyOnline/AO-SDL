/**
 * @file SpamDetector.h
 * @brief Cross-IP coordinated spam and abuse detection.
 *
 * Implements six heuristics that detect attack patterns invisible to
 * per-IP rate limiting:
 *
 *   H1  Message echo     — same message from 3+ IPIDs within 60s
 *   H2  Connection burst  — 20+ connections in 30s window
 *   H3  Join-and-spam     — <5s to first message + echo from 3+ IPs
 *   H5  Name pattern      — auto-generated username prefix clustering
 *   H6  Ghost connections  — connect but never identify, repeat from same IP
 *   H7  HWID reuse        — same HDID from 3+ different IPIDs
 *
 * Thread safety: all public methods are mutex-protected (single lock,
 * short critical sections). Memory is bounded via fixed-size ring buffers.
 */
#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/// Result of a spam detection check.
struct SpamVerdict {
    bool is_spam = false;
    std::string heuristic;  ///< Which heuristic triggered (e.g. "echo", "burst")
    std::string detail;     ///< Human-readable description
};

/// Configuration for the spam detector.
struct SpamDetectorConfig {
    bool enabled = true;

    // H1: Message echo
    int echo_threshold = 3;       ///< Unique IPIDs with same message
    int echo_window_seconds = 60; ///< Time window

    // H2: Connection burst
    // NOTE: This is a global counter, not per-ASN. During high-traffic
    // events (popular court sessions), legitimate users may trip this.
    // Tune the threshold higher for busy servers or combine with a
    // secondary signal (echo, ASN concentration) before auto-blocking.
    int burst_threshold = 20;      ///< Connections in window
    int burst_window_seconds = 30; ///< Time window

    // H3: Join-and-spam (uses echo_threshold, window is implicit)
    int join_spam_max_seconds = 5; ///< Max time from join to first message

    // H5: Name pattern
    int name_pattern_threshold = 3; ///< Unique IPIDs with same prefix
    int name_pattern_min_prefix = 4; ///< Minimum prefix length
    int name_pattern_window_seconds = 300; ///< 5 minutes

    // H6: Ghost connections
    int ghost_threshold = 5; ///< Failed-to-identify connections from one IP

    // H7: HWID reuse
    int hwid_reuse_threshold = 3; ///< Distinct IPIDs per HWID

    // Ring buffer sizes
    int message_ring_size = 1000;
    int name_ring_size = 200;
};

/// Callback invoked when spam is detected. Provides ASN for reputation reporting.
using SpamCallback = std::function<void(const std::string& ipid, uint32_t asn, const SpamVerdict& verdict)>;

class SpamDetector {
  public:
    SpamDetector() = default;
    ~SpamDetector() = default;

    SpamDetector(const SpamDetector&) = delete;
    SpamDetector& operator=(const SpamDetector&) = delete;

    /// Apply configuration.
    void configure(const SpamDetectorConfig& config);

    /// Set callback for detected spam events.
    void set_callback(SpamCallback callback);

    /// H1/H3: Check a message (OOC or IC global). Returns verdict.
    /// Called from CT/MS packet handlers.
    SpamVerdict check_message(const std::string& ipid, uint32_t asn, const std::string& message);

    /// H2/H5/H7: Register a new connection.
    /// Called from HI packet handler.
    SpamVerdict on_connection(const std::string& ipid, uint32_t asn, const std::string& hwid,
                               const std::string& username);

    /// H6: Register a disconnection. If the client never sent HI, counts as ghost.
    /// Called from the disconnect callback.
    void on_disconnect(const std::string& ip, bool completed_handshake);

    /// Record when a client joins (for H3 join-and-spam timing).
    void record_join_time(const std::string& ipid);

    /// Periodic cleanup. Call every 30s.
    void sweep();

  private:
    SpamDetectorConfig config_;
    SpamCallback callback_;
    mutable std::mutex mutex_;

    // --- H1: Message echo ---
    struct MessageRecord {
        size_t fingerprint;
        std::string ipid;
        uint32_t asn;
        int64_t timestamp; ///< steady_clock milliseconds
    };
    std::vector<MessageRecord> message_ring_;
    int message_ring_pos_ = 0;

    struct FingerprintCluster {
        int count = 0;
        std::unordered_set<std::string> unique_ipids;
        std::unordered_set<uint32_t> unique_asns;
        int64_t first_seen = 0;
        int64_t last_seen = 0;
        bool alerted = false; ///< Already triggered an alert
    };
    std::unordered_map<size_t, FingerprintCluster> fingerprint_clusters_;

    /// Normalize a message for fingerprinting (lowercase, collapse whitespace).
    static size_t fingerprint(const std::string& message);

    // --- H2: Connection burst ---
    std::vector<int64_t> connection_times_; ///< Recent connection timestamps

    // --- H3: Join-and-spam ---
    std::unordered_map<std::string, int64_t> join_times_; ///< ipid → join timestamp

    // --- H5: Name pattern ---
    struct NameRecord {
        std::string prefix;
        std::string ipid;
        int64_t timestamp;
    };
    std::vector<NameRecord> name_ring_;
    int name_ring_pos_ = 0;

    struct PrefixCluster {
        std::unordered_set<std::string> unique_ipids;
        int64_t first_seen = 0;
        int64_t last_seen = 0;
        bool alerted = false;
    };
    std::unordered_map<std::string, PrefixCluster> prefix_clusters_;

    /// Extract alphabetic prefix from a username (strip trailing digits).
    static std::string extract_prefix(const std::string& name);

    // --- H6: Ghost connections ---
    std::unordered_map<std::string, int> ghost_counts_; ///< IP → count of ghost connections

    // --- H7: HWID reuse ---
    struct HWIDRecord {
        std::unordered_set<std::string> unique_ipids;
        int64_t last_seen = 0;
        bool alerted = false;
    };
    std::unordered_map<std::string, HWIDRecord> hwid_map_;

    int64_t now_ms() const;
    void invoke_callback(const std::string& ipid, uint32_t asn, const SpamVerdict& verdict);
};
