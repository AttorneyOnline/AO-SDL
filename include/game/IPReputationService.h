/**
 * @file IPReputationService.h
 * @brief Async IP reputation lookup with caching and JSON persistence.
 *
 * Queries external services (ip-api.com, AbuseIPDB) for IP metadata:
 * ASN, country, ISP, proxy/datacenter flags, abuse confidence score.
 *
 * Results are cached in memory with configurable TTL and persisted to
 * a JSON file for warm restarts. Lookups are non-blocking — a dedicated
 * worker thread processes a batched queue.
 *
 * Thread safety: cache reads use a shared_mutex (read-heavy workload).
 * The lookup queue and writer use the same jthread/CV pattern as BanManager.
 */
#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>

struct IPReputationEntry {
    std::string ip;
    uint32_t asn = 0;
    std::string as_org;
    std::string country_code; ///< ISO 3166-1 alpha-2
    std::string isp;
    bool is_proxy = false;         ///< VPN, proxy, or Tor exit
    bool is_datacenter = false;    ///< Hosting / datacenter IP
    double abuse_confidence = 0.0; ///< 0.0–1.0 from AbuseIPDB
    int64_t fetched_at = 0;        ///< Unix seconds when fetched
    int64_t expires_at = 0;        ///< Unix seconds when cache entry expires
    bool lookup_failed = false;    ///< True if external query failed
};

/// Configuration for the reputation service.
struct ReputationConfig {
    bool enabled = true;
    int cache_ttl_hours = 24;
    int cache_failure_ttl_minutes = 5;
    bool ip_api_enabled = true;
    std::string abuseipdb_api_key;
    int abuseipdb_daily_budget = 1000;
    bool auto_block_proxy = false;
    bool auto_block_datacenter = false;
};

/// Callback invoked when an async lookup completes.
using ReputationCallback = std::function<void(const IPReputationEntry&)>;

class IPReputationService {
  public:
    IPReputationService();
    ~IPReputationService();

    IPReputationService(const IPReputationService&) = delete;
    IPReputationService& operator=(const IPReputationService&) = delete;

    /// Apply configuration. Safe to call at any time (hot-reload).
    void configure(const ReputationConfig& config);

    /// Load cached entries from a JSON file.
    void load(const std::string& path);

    /// Enqueue an async save of the cache to disk.
    void save_async(const std::string& path);

    /// Synchronous save. Blocks until the file is written.
    void save_sync(const std::string& path) const;

    /// Look up an IP. If cached and not expired, returns the entry immediately.
    /// Otherwise returns nullopt and enqueues an async lookup; the callback
    /// will be invoked on the worker thread when the result is ready.
    std::optional<IPReputationEntry> lookup(const std::string& ip, ReputationCallback callback = nullptr);

    /// Synchronous cache-only lookup. Never triggers an external query.
    std::optional<IPReputationEntry> find_cached(const std::string& ip) const;

    /// Insert or update a cache entry directly (e.g. from admin command).
    void put(IPReputationEntry entry);

    /// Remove expired entries from the cache. Returns count removed.
    size_t sweep_expired();

    /// Number of cached entries.
    size_t cache_size() const;

    /// Remaining AbuseIPDB budget for today.
    int abuseipdb_budget_remaining() const;

  private:
    ReputationConfig config_;

    // --- Cache ---
    mutable std::shared_mutex cache_mutex_;
    std::unordered_map<std::string, IPReputationEntry> cache_;

    // --- Lookup queue (batched ip-api.com queries) ---
    struct PendingLookup {
        std::string ip;
        ReputationCallback callback;
    };
    // queue_mutex_ and queue_cv_ must be declared BEFORE worker_thread_
    // because members initialize in declaration order, and the jthread
    // starts immediately.
    std::mutex queue_mutex_;
    std::condition_variable_any queue_cv_;
    std::deque<PendingLookup> queue_;

    // Declared after its synchronization primitives:
    std::jthread worker_thread_;

    void worker_loop(std::stop_token stop);

    /// Process a batch of IPs via ip-api.com /batch endpoint.
    /// Returns entries for each IP (may include failures).
    std::vector<IPReputationEntry> query_ip_api_batch(const std::vector<std::string>& ips);

    /// Query AbuseIPDB for a single IP. Returns updated entry or nullopt.
    std::optional<IPReputationEntry> query_abuseipdb(const std::string& ip);

    // --- AbuseIPDB daily budget ---
    std::atomic<int> abuseipdb_used_{0};
    std::chrono::system_clock::time_point abuseipdb_reset_day_;

    // --- Persistence (same pattern as BanManager) ---
    // writer_mutex_ and writer_cv_ must be declared BEFORE writer_thread_.
    std::mutex writer_mutex_;
    std::condition_variable_any writer_cv_;
    bool writer_pending_ = false;
    std::string writer_path_;
    std::unordered_map<std::string, IPReputationEntry> writer_snapshot_;
    std::jthread writer_thread_;

    void writer_loop(std::stop_token stop);
    std::unordered_map<std::string, IPReputationEntry> snapshot() const;
    static void write_to_disk(const std::unordered_map<std::string, IPReputationEntry>& cache, const std::string& path);

    int64_t now_unix() const;
    int64_t make_expiry(bool failed) const;
};
