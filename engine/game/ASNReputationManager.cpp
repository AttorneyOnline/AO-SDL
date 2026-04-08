#include "game/ASNReputationManager.h"

#include "game/BanManager.h" // parse_ban_duration
#include "metrics/MetricsRegistry.h"
#include "utils/Log.h"

#include <algorithm>
#include <fstream>
#include <json.hpp>

const char* asn_status_to_string(ASNReputationEntry::Status s) {
    switch (s) {
    case ASNReputationEntry::Status::NORMAL:
        return "NORMAL";
    case ASNReputationEntry::Status::WATCHED:
        return "WATCHED";
    case ASNReputationEntry::Status::RATE_LIMITED:
        return "RATE_LIMITED";
    case ASNReputationEntry::Status::BLOCKED:
        return "BLOCKED";
    }
    return "UNKNOWN";
}

// -- Helpers ------------------------------------------------------------------

int64_t ASNReputationManager::now_unix() const {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

bool ASNReputationManager::is_whitelisted(uint32_t asn) const {
    return std::ranges::find(config_.whitelist_asns, asn) != config_.whitelist_asns.end();
}

int ASNReputationManager::effective_threshold(int base_threshold, uint32_t asn) const {
    return is_whitelisted(asn) ? base_threshold * config_.whitelist_multiplier : base_threshold;
}

void ASNReputationManager::prune_window(ASNReputationEntry& entry) const {
    int64_t window_start = now_unix() - (config_.window_minutes * 60);
    std::erase_if(entry.recent_event_times, [window_start](int64_t t) { return t < window_start; });
}

// -- Construction / destruction -----------------------------------------------

ASNReputationManager::ASNReputationManager() : writer_thread_([this](std::stop_token st) { writer_loop(st); }) {
}

ASNReputationManager::~ASNReputationManager() {
    writer_thread_.request_stop();
    writer_cv_.notify_one();
}

void ASNReputationManager::configure(const ASNReputationConfig& config) {
    std::lock_guard lock(mutex_);
    config_ = config;
}

void ASNReputationManager::set_status_callback(ASNStatusCallback callback) {
    std::lock_guard lock(mutex_);
    status_callback_ = std::move(callback);
}

// -- Core operations ----------------------------------------------------------

void ASNReputationManager::report_abuse(uint32_t asn, const std::string& ip, const std::string& as_org,
                                         const std::string& reason) {
    if (!config_.enabled || asn == 0)
        return;

    ASNStatusCallback callback_to_invoke;
    ASNReputationEntry::Status old_status;
    ASNReputationEntry::Status new_status;
    uint32_t callback_asn = 0;
    std::string callback_reason;

    {
        std::lock_guard lock(mutex_);
        auto& entry = asns_[asn];

        // Initialize if new
        if (entry.asn == 0) {
            entry.asn = asn;
            entry.as_org = as_org;
            entry.status = ASNReputationEntry::Status::NORMAL;
        }
        else if (entry.as_org.empty() && !as_org.empty()) {
            entry.as_org = as_org;
        }

        // Skip if already blocked
        if (entry.status == ASNReputationEntry::Status::BLOCKED)
            return;

        auto now = now_unix();
        entry.total_abuse_events++;
        entry.recent_event_times.push_back(now);
        entry.abusive_ips_in_window.insert(ip);

        // Prune old events
        prune_window(entry);

        // Evaluate escalation
        old_status = entry.status;
        evaluate_escalation(entry);
        new_status = entry.status;

        if (old_status != new_status && status_callback_) {
            callback_to_invoke = status_callback_;
            callback_asn = asn;
            callback_reason = reason;
        }

        Log::log_print(INFO, "ASNReputation: AS%u (%s) abuse from %s: %s [%d unique IPs, status=%s]", asn,
                       entry.as_org.c_str(), ip.c_str(), reason.c_str(),
                       static_cast<int>(entry.abusive_ips_in_window.size()), asn_status_to_string(entry.status));

        static auto& abuse_counter =
            metrics::MetricsRegistry::instance().counter("kagami_asn_abuse_events_total", "ASN abuse events");
        abuse_counter.get().inc();
    }

    // Invoke callback outside the lock
    if (callback_to_invoke)
        callback_to_invoke(callback_asn, old_status, new_status, callback_reason);
}

void ASNReputationManager::evaluate_escalation(ASNReputationEntry& entry) {
    int unique_ips = static_cast<int>(entry.abusive_ips_in_window.size());
    auto now = now_unix();

    int watch_thresh = effective_threshold(config_.watch_threshold, entry.asn);
    int rate_limit_thresh = effective_threshold(config_.rate_limit_threshold, entry.asn);
    int block_thresh = effective_threshold(config_.block_threshold, entry.asn);

    auto escalate = [&](ASNReputationEntry::Status new_status, const char* reason) {
        Log::log_print(WARNING, "ASNReputation: AS%u (%s) escalated to %s — %s (%d unique abusive IPs)", entry.asn,
                       entry.as_org.c_str(), asn_status_to_string(new_status), reason, unique_ips);
        entry.status = new_status;
        entry.status_changed_at = now;

        if (new_status == ASNReputationEntry::Status::BLOCKED) {
            int64_t duration = parse_ban_duration(config_.auto_block_duration);
            if (duration > 0)
                entry.block_expires_at = now + duration;
            else
                entry.block_expires_at = 0; // permanent
            entry.block_reason = reason;

            static auto& blocked_counter =
                metrics::MetricsRegistry::instance().counter("kagami_asn_blocked_total", "ASNs auto-blocked");
            blocked_counter.get().inc();
        }
    };

    switch (entry.status) {
    case ASNReputationEntry::Status::NORMAL:
        if (unique_ips >= watch_thresh)
            escalate(ASNReputationEntry::Status::WATCHED, "abuse threshold (WATCHED)");
        break;

    case ASNReputationEntry::Status::WATCHED:
        if (unique_ips >= rate_limit_thresh)
            escalate(ASNReputationEntry::Status::RATE_LIMITED, "abuse threshold (RATE_LIMITED)");
        break;

    case ASNReputationEntry::Status::RATE_LIMITED:
        if (unique_ips >= block_thresh)
            escalate(ASNReputationEntry::Status::BLOCKED, "abuse threshold (BLOCKED)");
        // Also check sustained abuse: if rate-limited for >15 min and still getting abuse
        else if (now - entry.status_changed_at > 15 * 60 && !entry.recent_event_times.empty()) {
            // Check if there were events in the last 5 minutes
            int64_t recent_cutoff = now - 300;
            bool recent_abuse =
                std::ranges::any_of(entry.recent_event_times, [recent_cutoff](int64_t t) { return t > recent_cutoff; });
            if (recent_abuse)
                escalate(ASNReputationEntry::Status::BLOCKED, "sustained abuse after rate limiting");
        }
        break;

    case ASNReputationEntry::Status::BLOCKED:
        break; // Already at max
    }
}

std::optional<ASNReputationEntry> ASNReputationManager::check_blocked(uint32_t asn) const {
    if (asn == 0)
        return std::nullopt;
    std::lock_guard lock(mutex_);
    auto it = asns_.find(asn);
    if (it == asns_.end())
        return std::nullopt;
    if (it->second.status != ASNReputationEntry::Status::BLOCKED)
        return std::nullopt;
    // Check expiry
    if (it->second.block_expires_at > 0 && it->second.block_expires_at <= now_unix())
        return std::nullopt;
    return it->second;
}

std::optional<ASNReputationEntry> ASNReputationManager::get_status(uint32_t asn) const {
    std::lock_guard lock(mutex_);
    auto it = asns_.find(asn);
    if (it == asns_.end())
        return std::nullopt;
    return it->second;
}

void ASNReputationManager::block_asn(uint32_t asn, const std::string& as_org, const std::string& reason,
                                      int64_t duration_sec) {
    std::lock_guard lock(mutex_);
    auto& entry = asns_[asn];
    entry.asn = asn;
    if (!as_org.empty())
        entry.as_org = as_org;
    entry.status = ASNReputationEntry::Status::BLOCKED;
    entry.status_changed_at = now_unix();
    entry.block_expires_at = (duration_sec == -2) ? 0 : (now_unix() + duration_sec);
    entry.block_reason = reason;

    Log::log_print(INFO, "ASNReputation: AS%u (%s) manually blocked: %s", asn, entry.as_org.c_str(), reason.c_str());
}

bool ASNReputationManager::unblock_asn(uint32_t asn) {
    std::lock_guard lock(mutex_);
    auto it = asns_.find(asn);
    if (it == asns_.end())
        return false;
    it->second.status = ASNReputationEntry::Status::NORMAL;
    it->second.block_expires_at = 0;
    it->second.block_reason.clear();
    it->second.abusive_ips_in_window.clear();
    it->second.recent_event_times.clear();
    it->second.status_changed_at = now_unix();

    Log::log_print(INFO, "ASNReputation: AS%u (%s) unblocked", asn, it->second.as_org.c_str());
    return true;
}

std::vector<ASNReputationEntry> ASNReputationManager::list_flagged() const {
    std::lock_guard lock(mutex_);
    std::vector<ASNReputationEntry> result;
    for (auto& [_, entry] : asns_) {
        if (entry.status != ASNReputationEntry::Status::NORMAL)
            result.push_back(entry);
    }
    return result;
}

size_t ASNReputationManager::sweep() {
    std::lock_guard lock(mutex_);
    auto now = now_unix();
    size_t reset_count = 0;

    for (auto it = asns_.begin(); it != asns_.end();) {
        auto& entry = it->second;

        // Remove expired blocks
        if (entry.status == ASNReputationEntry::Status::BLOCKED && entry.block_expires_at > 0 &&
            entry.block_expires_at <= now) {
            Log::log_print(INFO, "ASNReputation: AS%u (%s) block expired, resetting to NORMAL", entry.asn,
                           entry.as_org.c_str());
            entry.status = ASNReputationEntry::Status::NORMAL;
            entry.block_reason.clear();
            entry.status_changed_at = now;
            ++reset_count;
        }

        // Prune old events
        prune_window(entry);

        // Also prune abusive_ips_in_window if no recent events remain
        if (entry.recent_event_times.empty())
            entry.abusive_ips_in_window.clear();

        // Remove entries with no abuse history and NORMAL status
        if (entry.status == ASNReputationEntry::Status::NORMAL && entry.total_abuse_events == 0 &&
            entry.abusive_ips_in_window.empty()) {
            it = asns_.erase(it);
        }
        else {
            ++it;
        }
    }

    return reset_count;
}

size_t ASNReputationManager::count() const {
    std::lock_guard lock(mutex_);
    return asns_.size();
}

// -- Persistence --------------------------------------------------------------

void ASNReputationManager::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        Log::log_print(INFO, "ASNReputation: no file at %s, starting fresh", path.c_str());
        return;
    }

    try {
        nlohmann::json j;
        file >> j;

        if (!j.is_array()) {
            Log::log_print(WARNING, "ASNReputation: %s is not a JSON array", path.c_str());
            return;
        }

        std::lock_guard lock(mutex_);
        asns_.clear();
        int loaded = 0;

        for (auto& item : j) {
            ASNReputationEntry entry;
            entry.asn = item.value("asn", uint32_t(0));
            entry.as_org = item.value("as_org", "");
            entry.total_abuse_events = item.value("total_abuse_events", 0);
            entry.status_changed_at = item.value("status_changed_at", int64_t(0));
            entry.block_expires_at = item.value("block_expires_at", int64_t(0));
            entry.block_reason = item.value("block_reason", "");

            std::string status_str = item.value("status", "NORMAL");
            if (status_str == "WATCHED")
                entry.status = ASNReputationEntry::Status::WATCHED;
            else if (status_str == "RATE_LIMITED")
                entry.status = ASNReputationEntry::Status::RATE_LIMITED;
            else if (status_str == "BLOCKED")
                entry.status = ASNReputationEntry::Status::BLOCKED;
            else
                entry.status = ASNReputationEntry::Status::NORMAL;

            if (entry.asn == 0)
                continue;

            // Check if block expired during downtime
            if (entry.status == ASNReputationEntry::Status::BLOCKED && entry.block_expires_at > 0 &&
                entry.block_expires_at <= now_unix()) {
                entry.status = ASNReputationEntry::Status::NORMAL;
                entry.block_reason.clear();
            }

            asns_[entry.asn] = std::move(entry);
            ++loaded;
        }

        Log::log_print(INFO, "ASNReputation: loaded %d entries from %s", loaded, path.c_str());
    }
    catch (const std::exception& e) {
        Log::log_print(WARNING, "ASNReputation: failed to parse %s: %s", path.c_str(), e.what());
    }
}

std::unordered_map<uint32_t, ASNReputationEntry> ASNReputationManager::snapshot() const {
    std::lock_guard lock(mutex_);
    return asns_;
}

void ASNReputationManager::write_to_disk(const std::unordered_map<uint32_t, ASNReputationEntry>& asns,
                                          const std::string& path) {
    try {
        nlohmann::json j = nlohmann::json::array();
        for (auto& [_, entry] : asns) {
            j.push_back({
                {"asn", entry.asn},
                {"as_org", entry.as_org},
                {"total_abuse_events", entry.total_abuse_events},
                {"status", asn_status_to_string(entry.status)},
                {"status_changed_at", entry.status_changed_at},
                {"block_expires_at", entry.block_expires_at},
                {"block_reason", entry.block_reason},
            });
        }

        std::ofstream file(path);
        if (file.is_open()) {
            file << j.dump(2) << std::endl;
            Log::log_print(DEBUG, "ASNReputation: saved %zu entries to %s", j.size(), path.c_str());
        }
        else {
            Log::log_print(WARNING, "ASNReputation: failed to open %s for writing", path.c_str());
        }
    }
    catch (const std::exception& e) {
        Log::log_print(WARNING, "ASNReputation: save failed: %s", e.what());
    }
}

void ASNReputationManager::save_async(const std::string& path) {
    {
        std::lock_guard lock(writer_mutex_);
        writer_path_ = path;
        writer_snapshot_ = snapshot();
        writer_pending_ = true;
    }
    writer_cv_.notify_one();
}

void ASNReputationManager::save_sync(const std::string& path) const {
    write_to_disk(snapshot(), path);
}

void ASNReputationManager::writer_loop(std::stop_token stop) {
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

    std::lock_guard lock(writer_mutex_);
    if (writer_pending_) {
        write_to_disk(writer_snapshot_, writer_path_);
        writer_pending_ = false;
    }
}
