#include "game/IPReputationService.h"

#include "metrics/MetricsRegistry.h"
#include "utils/Log.h"

#include <algorithm>
#include <fstream>
#include <json.hpp>
#include <ranges>

// Lightweight HTTP for outbound queries. We use the existing http::Client
// which provides a blocking API — fine for our dedicated worker thread.
#include "net/Http.h"

// -- Helpers ------------------------------------------------------------------

int64_t IPReputationService::now_unix() const {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

int64_t IPReputationService::make_expiry(bool failed) const {
    int64_t ttl = failed ? (config_.cache_failure_ttl_minutes * 60) : (config_.cache_ttl_hours * 3600);
    return now_unix() + ttl;
}

// -- Construction / destruction -----------------------------------------------

IPReputationService::IPReputationService()
    : abuseipdb_reset_day_(std::chrono::system_clock::now()),
      worker_thread_([this](std::stop_token st) { worker_loop(st); }),
      writer_thread_([this](std::stop_token st) { writer_loop(st); }) {
}

IPReputationService::~IPReputationService() {
    worker_thread_.request_stop();
    queue_cv_.notify_one();
    writer_thread_.request_stop();
    writer_cv_.notify_one();
}

void IPReputationService::configure(const ReputationConfig& config) {
    config_ = config;
}

// -- Cache operations ---------------------------------------------------------

std::optional<IPReputationEntry> IPReputationService::find_cached(const std::string& ip) const {
    std::shared_lock lock(cache_mutex_);
    auto it = cache_.find(ip);
    if (it != cache_.end() && it->second.expires_at > now_unix())
        return it->second;
    return std::nullopt;
}

std::optional<IPReputationEntry> IPReputationService::lookup(const std::string& ip, ReputationCallback callback) {
    if (!config_.enabled)
        return std::nullopt;

    // Fast path: cache hit
    if (auto cached = find_cached(ip))
        return cached;

    // Enqueue async lookup
    if (callback || config_.ip_api_enabled) {
        std::lock_guard lock(queue_mutex_);
        // Don't enqueue duplicates
        bool already_queued = std::ranges::any_of(queue_, [&](const PendingLookup& p) { return p.ip == ip; });
        if (!already_queued) {
            queue_.push_back({ip, std::move(callback)});
            queue_cv_.notify_one();
        }
    }

    return std::nullopt;
}

void IPReputationService::put(IPReputationEntry entry) {
    std::unique_lock lock(cache_mutex_);
    cache_[entry.ip] = std::move(entry);
}

size_t IPReputationService::sweep_expired() {
    auto now = now_unix();
    std::unique_lock lock(cache_mutex_);
    size_t removed = 0;
    for (auto it = cache_.begin(); it != cache_.end();) {
        if (it->second.expires_at <= now) {
            it = cache_.erase(it);
            ++removed;
        }
        else {
            ++it;
        }
    }
    return removed;
}

size_t IPReputationService::cache_size() const {
    std::shared_lock lock(cache_mutex_);
    return cache_.size();
}

int IPReputationService::abuseipdb_budget_remaining() const {
    return std::max(0, config_.abuseipdb_daily_budget - abuseipdb_used_.load(std::memory_order_relaxed));
}

// -- Worker thread (batched lookups) ------------------------------------------

void IPReputationService::worker_loop(std::stop_token stop) {
    while (!stop.stop_requested()) {
        std::vector<PendingLookup> batch;

        {
            std::unique_lock lock(queue_mutex_);
            // Wait for work or a 2-second batch accumulation window
            queue_cv_.wait_for(lock, std::chrono::seconds(2), [&] { return !queue_.empty() || stop.stop_requested(); });

            if (stop.stop_requested())
                break;
            if (queue_.empty())
                continue;

            // Drain up to 100 items (ip-api.com batch limit)
            size_t count = std::min(queue_.size(), size_t(100));
            for (size_t i = 0; i < count; ++i) {
                batch.push_back(std::move(queue_.front()));
                queue_.pop_front();
            }
        }

        if (batch.empty())
            continue;

        // Collect IPs for batch query
        std::vector<std::string> ips;
        ips.reserve(batch.size());
        for (auto& item : batch)
            ips.push_back(item.ip);

        // Query ip-api.com
        auto results = query_ip_api_batch(ips);

        // Build a map for quick lookup
        std::unordered_map<std::string, IPReputationEntry> result_map;
        for (auto& entry : results)
            result_map[entry.ip] = entry;

        // Update cache and invoke callbacks
        for (auto& item : batch) {
            auto it = result_map.find(item.ip);
            if (it != result_map.end()) {
                put(it->second);
                if (item.callback)
                    item.callback(it->second);
            }
            else {
                // Create a failure entry
                IPReputationEntry failed;
                failed.ip = item.ip;
                failed.lookup_failed = true;
                failed.fetched_at = now_unix();
                failed.expires_at = make_expiry(true);
                put(failed);
                if (item.callback)
                    item.callback(failed);
            }
        }

        // Check if any results warrant AbuseIPDB follow-up
        if (!config_.abuseipdb_api_key.empty()) {
            // Reset daily budget if needed
            auto now = std::chrono::system_clock::now();
            auto today = std::chrono::floor<std::chrono::days>(now);
            auto reset_day = std::chrono::floor<std::chrono::days>(abuseipdb_reset_day_);
            if (today > reset_day) {
                abuseipdb_used_.store(0, std::memory_order_relaxed);
                abuseipdb_reset_day_ = now;
            }

            for (auto& entry : results) {
                // Query AbuseIPDB for datacenter/proxy IPs (higher risk)
                if ((entry.is_proxy || entry.is_datacenter) && abuseipdb_budget_remaining() > 0) {
                    auto enriched = query_abuseipdb(entry.ip);
                    if (enriched) {
                        put(*enriched);
                    }
                }
            }
        }
    }
}

// -- ip-api.com batch query ---------------------------------------------------

std::vector<IPReputationEntry> IPReputationService::query_ip_api_batch(const std::vector<std::string>& ips) {
    std::vector<IPReputationEntry> results;

    if (!config_.ip_api_enabled || ips.empty())
        return results;

    try {
        // Build JSON array for POST /batch
        nlohmann::json request_body = nlohmann::json::array();
        for (auto& ip : ips) {
            request_body.push_back({
                {"query", ip},
                {"fields", "query,status,country,countryCode,isp,org,as,proxy,hosting"},
            });
        }

        std::string body = request_body.dump();

        http::Client client("http://ip-api.com");
        auto res = client.Post("/batch", body, "application/json");

        if (!res || res->status != 200) {
            Log::log_print(WARNING, "IPReputation: ip-api.com batch query failed (status=%d)", res ? res->status : -1);
            return results;
        }

        auto response = nlohmann::json::parse(res->body);
        if (!response.is_array())
            return results;

        auto now = now_unix();

        for (auto& item : response) {
            if (item.value("status", "") != "success")
                continue;

            IPReputationEntry entry;
            entry.ip = item.value("query", "");
            entry.country_code = item.value("countryCode", "");
            entry.isp = item.value("isp", "");
            entry.as_org = item.value("org", "");
            entry.is_proxy = item.value("proxy", false);
            entry.is_datacenter = item.value("hosting", false);
            entry.fetched_at = now;
            entry.expires_at = make_expiry(false);

            // Parse ASN from "as" field: "AS12345 Organization Name"
            std::string as_str = item.value("as", "");
            if (as_str.size() > 2 && (as_str[0] == 'A' || as_str[0] == 'a') && (as_str[1] == 'S' || as_str[1] == 's')) {
                auto space = as_str.find(' ', 2);
                std::string num = (space != std::string::npos) ? as_str.substr(2, space - 2) : as_str.substr(2);
                try {
                    entry.asn = static_cast<uint32_t>(std::stoul(num));
                }
                catch (...) {
                    // stoul failed — leave asn as 0
                }
                if (entry.as_org.empty() && space != std::string::npos)
                    entry.as_org = as_str.substr(space + 1);
            }

            results.push_back(std::move(entry));
        }

        Log::log_print(DEBUG, "IPReputation: ip-api.com batch returned %zu/%zu results", results.size(), ips.size());

        static auto& lookup_counter = metrics::MetricsRegistry::instance().counter(
            "kagami_reputation_lookups_total", "IP reputation lookups", {"source", "status"});
        lookup_counter.labels({"ip_api", "success"}).inc(results.size());
        if (results.size() < ips.size())
            lookup_counter.labels({"ip_api", "error"}).inc(ips.size() - results.size());
    }
    catch (const std::exception& e) {
        Log::log_print(WARNING, "IPReputation: ip-api.com query error: %s", e.what());
    }

    return results;
}

// -- AbuseIPDB query ----------------------------------------------------------

std::optional<IPReputationEntry> IPReputationService::query_abuseipdb(const std::string& ip) {
    if (config_.abuseipdb_api_key.empty() || abuseipdb_budget_remaining() <= 0)
        return std::nullopt;

    abuseipdb_used_.fetch_add(1, std::memory_order_relaxed);

    try {
        http::Client client("https://api.abuseipdb.com");
        http::Headers headers = {
            {"Key", config_.abuseipdb_api_key},
            {"Accept", "application/json"},
        };

        std::string path = "/api/v2/check?ipAddress=" + ip + "&maxAgeInDays=90";
        auto res = client.Get(path, headers);

        if (!res || res->status != 200) {
            Log::log_print(WARNING, "IPReputation: AbuseIPDB query failed for %s (status=%d)", ip.c_str(),
                           res ? res->status : -1);
            return std::nullopt;
        }

        auto response = nlohmann::json::parse(res->body);
        auto& data = response["data"];

        // Start from existing cached entry if available, else create new
        IPReputationEntry entry;
        if (auto cached = find_cached(ip))
            entry = *cached;
        else
            entry.ip = ip;

        entry.abuse_confidence = data.value("abuseConfidenceScore", 0) / 100.0;
        entry.fetched_at = now_unix();
        entry.expires_at = make_expiry(false);

        // AbuseIPDB also provides some fields we can fill in
        if (entry.country_code.empty())
            entry.country_code = data.value("countryCode", "");
        if (entry.isp.empty())
            entry.isp = data.value("isp", "");

        Log::log_print(DEBUG, "IPReputation: AbuseIPDB %s -> confidence=%.0f%%", ip.c_str(),
                       entry.abuse_confidence * 100.0);

        return entry;
    }
    catch (const std::exception& e) {
        Log::log_print(WARNING, "IPReputation: AbuseIPDB error for %s: %s", ip.c_str(), e.what());
        return std::nullopt;
    }
}

// -- Persistence --------------------------------------------------------------

void IPReputationService::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        Log::log_print(INFO, "IPReputation: no cache file at %s, starting fresh", path.c_str());
        return;
    }

    try {
        nlohmann::json j;
        file >> j;

        if (!j.is_array()) {
            Log::log_print(WARNING, "IPReputation: %s is not a JSON array", path.c_str());
            return;
        }

        auto now = now_unix();
        std::unique_lock lock(cache_mutex_);
        cache_.clear();
        int loaded = 0;
        int skipped = 0;

        for (auto& item : j) {
            IPReputationEntry entry;
            entry.ip = item.value("ip", "");
            entry.asn = item.value("asn", uint32_t(0));
            entry.as_org = item.value("as_org", "");
            entry.country_code = item.value("country_code", "");
            entry.isp = item.value("isp", "");
            entry.is_proxy = item.value("is_proxy", false);
            entry.is_datacenter = item.value("is_datacenter", false);
            entry.abuse_confidence = item.value("abuse_confidence", 0.0);
            entry.fetched_at = item.value("fetched_at", int64_t(0));
            entry.expires_at = item.value("expires_at", int64_t(0));
            entry.lookup_failed = item.value("lookup_failed", false);

            if (entry.ip.empty())
                continue;
            if (entry.expires_at <= now) {
                ++skipped;
                continue;
            }

            cache_[entry.ip] = std::move(entry);
            ++loaded;
        }

        Log::log_print(INFO, "IPReputation: loaded %d entries from %s (%d expired, skipped)", loaded, path.c_str(),
                       skipped);
    }
    catch (const std::exception& e) {
        Log::log_print(WARNING, "IPReputation: failed to parse %s: %s", path.c_str(), e.what());
    }
}

std::unordered_map<std::string, IPReputationEntry> IPReputationService::snapshot() const {
    std::shared_lock lock(cache_mutex_);
    return cache_;
}

void IPReputationService::write_to_disk(const std::unordered_map<std::string, IPReputationEntry>& cache,
                                        const std::string& path) {
    try {
        auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                       .count();

        nlohmann::json j = nlohmann::json::array();
        for (auto& [_, entry] : cache) {
            if (entry.expires_at <= now)
                continue;
            j.push_back({
                {"ip", entry.ip},
                {"asn", entry.asn},
                {"as_org", entry.as_org},
                {"country_code", entry.country_code},
                {"isp", entry.isp},
                {"is_proxy", entry.is_proxy},
                {"is_datacenter", entry.is_datacenter},
                {"abuse_confidence", entry.abuse_confidence},
                {"fetched_at", entry.fetched_at},
                {"expires_at", entry.expires_at},
                {"lookup_failed", entry.lookup_failed},
            });
        }

        std::ofstream file(path);
        if (file.is_open()) {
            file << j.dump(2) << std::endl;
            Log::log_print(DEBUG, "IPReputation: saved %zu entries to %s", j.size(), path.c_str());
        }
        else {
            Log::log_print(WARNING, "IPReputation: failed to open %s for writing", path.c_str());
        }
    }
    catch (const std::exception& e) {
        Log::log_print(WARNING, "IPReputation: save failed: %s", e.what());
    }
}

void IPReputationService::save_async(const std::string& path) {
    {
        std::lock_guard lock(writer_mutex_);
        writer_path_ = path;
        writer_snapshot_ = snapshot();
        writer_pending_ = true;
    }
    writer_cv_.notify_one();
}

void IPReputationService::save_sync(const std::string& path) const {
    write_to_disk(snapshot(), path);
}

void IPReputationService::writer_loop(std::stop_token stop) {
    while (!stop.stop_requested()) {
        std::unique_lock lock(writer_mutex_);
        writer_cv_.wait(lock, [&] { return writer_pending_ || stop.stop_requested(); });

        if (writer_pending_) {
            auto snap = std::move(writer_snapshot_);
            auto path = std::move(writer_path_);
            writer_pending_ = false;
            lock.unlock();

            write_to_disk(snap, path);
        }
    }

    // Flush any final pending write on shutdown
    std::lock_guard lock(writer_mutex_);
    if (writer_pending_) {
        write_to_disk(writer_snapshot_, writer_path_);
        writer_pending_ = false;
    }
}
