#include "game/BanManager.h"

#include "game/DatabaseManager.h"
#include "game/FirewallManager.h"
#include "utils/Log.h"

#include <algorithm>
#include <cctype>

// -- Duration parsing ---------------------------------------------------------

int64_t parse_ban_duration(const std::string& input) {
    if (input.empty())
        return -1;

    // Check for "perma" / "permanent"
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    if (lower == "perma" || lower == "permanent")
        return -2;

    // Parse compound duration: "2h30m15s"
    int64_t total = 0;
    int64_t current_num = 0;
    bool has_unit = false;

    for (char c : lower) {
        if (std::isdigit(c)) {
            current_num = current_num * 10 + (c - '0');
        }
        else if (c == 'h') {
            total += current_num * 3600;
            current_num = 0;
            has_unit = true;
        }
        else if (c == 'm') {
            total += current_num * 60;
            current_num = 0;
            has_unit = true;
        }
        else if (c == 's') {
            total += current_num;
            current_num = 0;
            has_unit = true;
        }
        else {
            return -1; // invalid character
        }
    }

    // Trailing number with no unit: treat as seconds
    if (current_num > 0) {
        total += current_num;
        has_unit = true;
    }

    return has_unit ? total : -1;
}

std::string format_ban_duration(int64_t duration) {
    if (duration == -2)
        return "permanent";
    if (duration <= 0)
        return "invalidated";

    std::string result;
    int64_t h = duration / 3600;
    int64_t m = (duration % 3600) / 60;
    int64_t s = duration % 60;
    if (h > 0)
        result += std::to_string(h) + "h";
    if (m > 0)
        result += std::to_string(m) + "m";
    if (s > 0 || result.empty())
        result += std::to_string(s) + "s";
    return result;
}

// -- BanManager ---------------------------------------------------------------

void BanManager::load_from_db() {
    if (!db_ || !db_->is_open())
        return;

    auto bans = db_->all_active_bans().get(); // blocks — startup only

    std::lock_guard lock(mutex_);
    bans_.clear();
    int fw_synced = 0;
    for (auto& entry : bans) {
        // Sync firewall state for bans with known IPs
        if (fw_ && fw_->is_enabled() && !entry.ip.empty()) {
            int64_t fw_dur = entry.is_permanent() ? DURATION_PERMANENT : entry.duration;
            fw_->block_ip(entry.ip, entry.reason, fw_dur);
            ++fw_synced;
        }
        bans_[entry.ipid] = std::move(entry);
    }

    Log::log_print(INFO, "BanManager: loaded %zu active bans from database (%d firewall rules synced)", bans_.size(),
                   fw_synced);
}

void BanManager::add_ban(BanEntry entry) {
    // Firewall block (before taking the lock — fw has its own mutex)
    if (fw_ && fw_->is_enabled() && !entry.ip.empty()) {
        int64_t fw_dur = entry.is_permanent() ? DURATION_PERMANENT : entry.duration;
        fw_->block_ip(entry.ip, entry.reason, fw_dur);
    }

    std::lock_guard lock(mutex_);
    auto ipid = entry.ipid;
    bans_[ipid] = entry;

    if (db_) {
        db_->add_ban(std::move(entry));
    }
}

bool BanManager::remove_ban(const std::string& ipid) {
    std::lock_guard lock(mutex_);
    auto it = bans_.find(ipid);
    if (it == bans_.end())
        return false;

    int64_t ban_id = it->second.id;
    std::string ip = it->second.ip;
    bans_.erase(it);

    if (db_ && ban_id > 0)
        db_->invalidate_ban(ban_id);

    // Firewall unblock (after releasing map entry, fw has its own mutex)
    if (fw_ && fw_->is_enabled() && !ip.empty())
        fw_->unblock_ip(ip);

    return true;
}

bool BanManager::update_ban(const std::string& ipid, const std::string& field, const std::string& value) {
    std::lock_guard lock(mutex_);
    auto it = bans_.find(ipid);
    if (it == bans_.end())
        return false;

    // Update in-memory
    if (field == "reason") {
        it->second.reason = value;
    }
    else if (field == "duration") {
        int64_t dur = parse_ban_duration(value);
        if (dur == -1)
            return false;
        it->second.duration = dur;
    }
    else {
        return false;
    }

    // Write-through to DB
    if (db_ && it->second.id > 0) {
        std::string db_value = value;
        if (field == "duration")
            db_value = std::to_string(it->second.duration);
        db_->update_ban(it->second.id, field, db_value);
    }

    // Update firewall rule if duration changed
    if (field == "duration" && fw_ && fw_->is_enabled() && !it->second.ip.empty()) {
        fw_->unblock_ip(it->second.ip);
        int64_t fw_dur = it->second.is_permanent() ? DURATION_PERMANENT : it->second.duration;
        fw_->block_ip(it->second.ip, it->second.reason, fw_dur);
    }

    return true;
}

std::optional<BanEntry> BanManager::find_ban(const std::string& ipid) const {
    std::lock_guard lock(mutex_);
    auto it = bans_.find(ipid);
    if (it != bans_.end() && !it->second.is_expired())
        return it->second;
    return std::nullopt;
}

std::optional<BanEntry> BanManager::find_ban_by_hdid(const std::string& hdid) const {
    if (hdid.empty())
        return std::nullopt;
    std::lock_guard lock(mutex_);
    for (auto& [_, entry] : bans_) {
        if (!entry.hdid.empty() && entry.hdid == hdid && !entry.is_expired())
            return entry;
    }
    return std::nullopt;
}

std::optional<BanEntry> BanManager::check_ban(const std::string& ipid, const std::string& hdid) const {
    std::lock_guard lock(mutex_);

    auto it = bans_.find(ipid);
    if (it != bans_.end() && !it->second.is_expired())
        return it->second;

    if (!hdid.empty()) {
        for (auto& [_, entry] : bans_) {
            if (!entry.hdid.empty() && entry.hdid == hdid && !entry.is_expired())
                return entry;
        }
    }

    return std::nullopt;
}

std::vector<BanEntry> BanManager::search_bans(const std::string& query, int limit) const {
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    auto contains = [&](const std::string& field) {
        std::string lower_field = field;
        std::transform(lower_field.begin(), lower_field.end(), lower_field.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return lower_field.find(lower_query) != std::string::npos;
    };

    std::lock_guard lock(mutex_);
    std::vector<BanEntry> results;

    for (auto& [_, entry] : bans_) {
        if (contains(entry.ipid) || contains(entry.hdid) || contains(entry.reason) || contains(entry.moderator)) {
            results.push_back(entry);
            if (static_cast<int>(results.size()) >= limit)
                break;
        }
    }

    return results;
}

std::vector<BanEntry> BanManager::list_bans(int limit) const {
    std::lock_guard lock(mutex_);
    std::vector<BanEntry> result;
    result.reserve(bans_.size());
    for (auto& [_, entry] : bans_)
        result.push_back(entry);

    // Sort by timestamp descending
    std::sort(result.begin(), result.end(),
              [](const BanEntry& a, const BanEntry& b) { return a.timestamp > b.timestamp; });

    if (static_cast<int>(result.size()) > limit)
        result.resize(limit);
    return result;
}

size_t BanManager::count() const {
    std::lock_guard lock(mutex_);
    size_t n = 0;
    for (auto& [_, entry] : bans_) {
        if (!entry.is_expired())
            ++n;
    }
    return n;
}
