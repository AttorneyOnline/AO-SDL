#include "game/SpamDetector.h"

#include "metrics/MetricsRegistry.h"
#include "utils/Log.h"

#include <cctype>
#include <chrono>
#include <functional>

// -- Helpers ------------------------------------------------------------------

int64_t SpamDetector::now_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

size_t SpamDetector::fingerprint(const std::string& message) {
    // Normalize: lowercase, collapse whitespace, trim
    std::string normalized;
    normalized.reserve(message.size());
    bool last_was_space = true;
    for (char c : message) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!last_was_space) {
                normalized.push_back(' ');
                last_was_space = true;
            }
        }
        else {
            normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            last_was_space = false;
        }
    }
    // Trim trailing space
    if (!normalized.empty() && normalized.back() == ' ')
        normalized.pop_back();

    return std::hash<std::string>{}(normalized);
}

std::string SpamDetector::extract_prefix(const std::string& name) {
    // Find the longest leading alphabetic prefix before digits
    // "TuskNail2638" → "TuskNail", "TWKTWK502" → "TWKTWK"
    size_t end = name.size();
    // Walk backwards past trailing digits
    while (end > 0 && std::isdigit(static_cast<unsigned char>(name[end - 1])))
        --end;
    // Must have at least some trailing digits to be a "pattern" name
    if (end == name.size() || end == 0)
        return ""; // No trailing digits or all digits — not a pattern
    return name.substr(0, end);
}

void SpamDetector::invoke_callback(const std::string& ipid, uint32_t asn, const SpamVerdict& verdict) {
    static auto& spam_counter =
        metrics::MetricsRegistry::instance().counter("kagami_spam_detected_total", "Spam events detected", {"type"});
    spam_counter.labels({verdict.heuristic}).inc();

    if (callback_)
        callback_(ipid, asn, verdict);
}

// -- Configuration ------------------------------------------------------------

void SpamDetector::configure(const SpamDetectorConfig& config) {
    std::lock_guard lock(mutex_);
    config_ = config;
    message_ring_.resize(config.message_ring_size);
    name_ring_.resize(config.name_ring_size);
}

void SpamDetector::set_callback(SpamCallback callback) {
    std::lock_guard lock(mutex_);
    callback_ = std::move(callback);
}

// -- H1/H3: Message check ----------------------------------------------------

SpamVerdict SpamDetector::check_message(const std::string& ipid, const std::string& ip, uint32_t asn,
                                        const std::string& message) {
    if (!config_.enabled)
        return {};

    auto fp = fingerprint(message);
    auto now = now_ms();

    SpamCallback cb;
    SpamVerdict verdict;

    {
        std::lock_guard lock(mutex_);

        // Record IPID → IP mapping so auto-ban can resolve participants.
        if (!ip.empty())
            ipid_to_ip_[ipid] = ip;

        // Record in ring buffer
        if (!message_ring_.empty()) {
            message_ring_[message_ring_pos_] = {fp, ipid, asn, now};
            message_ring_pos_ = (message_ring_pos_ + 1) % static_cast<int>(message_ring_.size());
        }

        // Update fingerprint cluster
        auto& cluster = fingerprint_clusters_[fp];
        cluster.count++;
        cluster.unique_ipids.insert(ipid);
        cluster.unique_asns.insert(asn);
        if (cluster.first_seen == 0)
            cluster.first_seen = now;
        cluster.last_seen = now;

        int window_ms = config_.echo_window_seconds * 1000;
        bool in_window = (now - cluster.first_seen) <= window_ms;
        int unique_count = static_cast<int>(cluster.unique_ipids.size());

        // H1: Message echo detection
        if (in_window && unique_count >= config_.echo_threshold && !cluster.alerted) {
            cluster.alerted = true;
            verdict.is_spam = true;
            verdict.heuristic = "echo";
            verdict.detail = "Same message from " + std::to_string(unique_count) + " distinct IPs within " +
                             std::to_string(config_.echo_window_seconds) + "s";

            // Collect all participants (ipid, ip) for auto-ban callback.
            verdict.participants.reserve(cluster.unique_ipids.size());
            for (auto& pid : cluster.unique_ipids) {
                auto it = ipid_to_ip_.find(pid);
                if (it != ipid_to_ip_.end() && !it->second.empty())
                    verdict.participants.emplace_back(pid, it->second);
            }

            Log::log_print(WARNING, "SpamDetector [H1 echo]: %s — %d unique IPs, fingerprint=%zu",
                           verdict.detail.c_str(), unique_count, fp);
            cb = callback_;
        }

        // H3: Join-and-spam detection
        if (!verdict.is_spam) {
            auto jt = join_times_.find(ipid);
            if (jt != join_times_.end()) {
                int64_t time_since_join_ms = now - jt->second;
                if (time_since_join_ms < config_.join_spam_max_seconds * 1000) {
                    // This client sent a message very quickly after joining.
                    // Check if there are other fast-joiners with the same message.
                    int fast_joiners = 0;
                    for (auto& uid : cluster.unique_ipids) {
                        auto jt2 = join_times_.find(uid);
                        if (jt2 != join_times_.end()) {
                            int64_t their_delay = cluster.last_seen - jt2->second;
                            if (their_delay < config_.join_spam_max_seconds * 1000)
                                ++fast_joiners;
                        }
                    }

                    if (fast_joiners >= config_.echo_threshold && !cluster.alerted) {
                        cluster.alerted = true;
                        verdict.is_spam = true;
                        verdict.heuristic = "join_spam";
                        verdict.detail = std::to_string(fast_joiners) + " IPs sent same message within " +
                                         std::to_string(config_.join_spam_max_seconds) + "s of joining";

                        // Collect participants (same IPIDs as echo — fast-joiners subset)
                        verdict.participants.reserve(cluster.unique_ipids.size());
                        for (auto& pid : cluster.unique_ipids) {
                            auto it = ipid_to_ip_.find(pid);
                            if (it != ipid_to_ip_.end() && !it->second.empty())
                                verdict.participants.emplace_back(pid, it->second);
                        }

                        Log::log_print(WARNING, "SpamDetector [H3 join-spam]: %s", verdict.detail.c_str());
                        cb = callback_;
                    }
                }
            }
        }
    }

    if (verdict.is_spam && cb)
        cb(ipid, asn, verdict);

    return verdict;
}

// -- H2/H5/H7: Connection registration ---------------------------------------

SpamVerdict SpamDetector::on_connection(const std::string& ipid, const std::string& ip, uint32_t asn,
                                        const std::string& hwid, const std::string& username) {
    if (!config_.enabled)
        return {};

    auto now = now_ms();
    SpamCallback cb;
    SpamVerdict verdict;

    {
        std::lock_guard lock(mutex_);

        // Record IPID → IP mapping so auto-ban can resolve participants.
        if (!ip.empty())
            ipid_to_ip_[ipid] = ip;

        // H2: Connection burst tracking
        connection_times_.push_back(now);
        int64_t burst_cutoff = now - (config_.burst_window_seconds * 1000);
        std::erase_if(connection_times_, [burst_cutoff](int64_t t) { return t < burst_cutoff; });

        if (static_cast<int>(connection_times_.size()) >= config_.burst_threshold) {
            verdict.is_spam = true;
            verdict.heuristic = "burst";
            verdict.detail = std::to_string(connection_times_.size()) + " connections in " +
                             std::to_string(config_.burst_window_seconds) + "s window";

            Log::log_print(WARNING, "SpamDetector [H2 burst]: %s", verdict.detail.c_str());
            cb = callback_;
        }

        // H5: Name pattern detection
        if (!username.empty()) {
            std::string prefix = extract_prefix(username);
            if (static_cast<int>(prefix.size()) >= config_.name_pattern_min_prefix) {
                // Record in name ring
                if (!name_ring_.empty()) {
                    name_ring_[name_ring_pos_] = {prefix, ipid, now};
                    name_ring_pos_ = (name_ring_pos_ + 1) % static_cast<int>(name_ring_.size());
                }

                auto& pc = prefix_clusters_[prefix];
                pc.unique_ipids.insert(ipid);
                if (pc.first_seen == 0)
                    pc.first_seen = now;
                pc.last_seen = now;

                int64_t name_window_ms = config_.name_pattern_window_seconds * 1000;
                bool in_window = (now - pc.first_seen) <= name_window_ms;
                int unique_count = static_cast<int>(pc.unique_ipids.size());

                if (in_window && unique_count >= config_.name_pattern_threshold && !pc.alerted) {
                    pc.alerted = true;

                    if (!verdict.is_spam) {
                        verdict.is_spam = true;
                        verdict.heuristic = "name_pattern";
                        verdict.detail =
                            "Username prefix \"" + prefix + "\" from " + std::to_string(unique_count) + " distinct IPs";

                        // Collect all IPIDs that shared this username prefix.
                        verdict.participants.reserve(pc.unique_ipids.size());
                        for (auto& pid : pc.unique_ipids) {
                            auto it = ipid_to_ip_.find(pid);
                            if (it != ipid_to_ip_.end() && !it->second.empty())
                                verdict.participants.emplace_back(pid, it->second);
                        }

                        Log::log_print(WARNING, "SpamDetector [H5 name-pattern]: %s", verdict.detail.c_str());
                        cb = callback_;
                    }
                }
            }
        }

        // H7: HWID reuse detection
        if (!hwid.empty()) {
            auto& hr = hwid_map_[hwid];
            hr.unique_ipids.insert(ipid);
            hr.last_seen = now;

            if (static_cast<int>(hr.unique_ipids.size()) >= config_.hwid_reuse_threshold && !hr.alerted) {
                hr.alerted = true;

                if (!verdict.is_spam) {
                    verdict.is_spam = true;
                    verdict.heuristic = "hwid_reuse";
                    verdict.detail = "HWID " + hwid.substr(0, 8) + "... seen from " +
                                     std::to_string(hr.unique_ipids.size()) + " distinct IPs";

                    Log::log_print(WARNING, "SpamDetector [H7 hwid-reuse]: %s", verdict.detail.c_str());
                    cb = callback_;
                }
            }
        }
    }

    if (verdict.is_spam && cb)
        cb(ipid, asn, verdict);

    return verdict;
}

// -- H6: Ghost connection tracking --------------------------------------------

void SpamDetector::on_disconnect(const std::string& ip, bool completed_handshake) {
    if (!config_.enabled || completed_handshake)
        return;

    std::lock_guard lock(mutex_);
    int& count = ghost_counts_[ip];
    ++count;

    if (count >= config_.ghost_threshold) {
        Log::log_print(WARNING, "SpamDetector [H6 ghost]: IP %s has %d ghost connections (never identified)",
                       ip.c_str(), count);
        // Ghost connections don't produce a SpamVerdict return since they're
        // detected at disconnect time. The log + callback is the action.
        if (callback_) {
            SpamVerdict v;
            v.is_spam = true;
            v.heuristic = "ghost";
            v.detail = "IP " + ip + " has " + std::to_string(count) + " ghost connections";
            callback_("", 0, v);
        }
    }
}

// -- Join time recording (for H3) ---------------------------------------------

void SpamDetector::record_join_time(const std::string& ipid) {
    std::lock_guard lock(mutex_);
    join_times_[ipid] = now_ms();
}

// -- Periodic cleanup ---------------------------------------------------------

void SpamDetector::sweep() {
    std::lock_guard lock(mutex_);
    auto now = now_ms();

    // Prune fingerprint clusters older than the echo window
    int64_t echo_cutoff = now - (config_.echo_window_seconds * 1000);
    for (auto it = fingerprint_clusters_.begin(); it != fingerprint_clusters_.end();) {
        if (it->second.last_seen < echo_cutoff)
            it = fingerprint_clusters_.erase(it);
        else
            ++it;
    }

    // Prune connection burst times (already pruned on access, but clean up here too)
    int64_t burst_cutoff = now - (config_.burst_window_seconds * 1000);
    std::erase_if(connection_times_, [burst_cutoff](int64_t t) { return t < burst_cutoff; });

    // Prune join times older than 5 minutes
    int64_t join_cutoff = now - 300000;
    for (auto it = join_times_.begin(); it != join_times_.end();) {
        if (it->second < join_cutoff)
            it = join_times_.erase(it);
        else
            ++it;
    }

    // Prune prefix clusters older than the name pattern window
    int64_t name_cutoff = now - (config_.name_pattern_window_seconds * 1000);
    for (auto it = prefix_clusters_.begin(); it != prefix_clusters_.end();) {
        if (it->second.last_seen < name_cutoff)
            it = prefix_clusters_.erase(it);
        else
            ++it;
    }

    // Prune ghost counts (reset every sweep — they accumulate over 30s periods)
    ghost_counts_.clear();

    // Prune HWID records older than 1 hour
    int64_t hwid_cutoff = now - 3600000;
    for (auto it = hwid_map_.begin(); it != hwid_map_.end();) {
        if (it->second.last_seen < hwid_cutoff)
            it = hwid_map_.erase(it);
        else
            ++it;
    }

    // Prune IPID → IP map. Only keep entries that are still referenced by
    // an active fingerprint or prefix cluster; everything else is stale.
    std::unordered_set<std::string> live_ipids;
    for (auto& [_, cluster] : fingerprint_clusters_)
        live_ipids.insert(cluster.unique_ipids.begin(), cluster.unique_ipids.end());
    for (auto& [_, pc] : prefix_clusters_)
        live_ipids.insert(pc.unique_ipids.begin(), pc.unique_ipids.end());
    for (auto it = ipid_to_ip_.begin(); it != ipid_to_ip_.end();) {
        if (live_ipids.find(it->first) == live_ipids.end())
            it = ipid_to_ip_.erase(it);
        else
            ++it;
    }
}
