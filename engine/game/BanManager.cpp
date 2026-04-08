#include "game/BanManager.h"

#include "utils/Log.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <json.hpp>

// -- Duration parsing ---------------------------------------------------------

int64_t parse_ban_duration(const std::string& input) {
    if (input.empty())
        return -1;

    // Check for "perma" / "permanent"
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
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

// -- BanManager ---------------------------------------------------------------

BanManager::BanManager() : writer_thread_([this](std::stop_token st) { writer_loop(st); }) {
}

BanManager::~BanManager() {
    // Signal the writer to stop and flush any pending write.
    writer_thread_.request_stop();
    writer_cv_.notify_one();
    // jthread destructor joins automatically.
}

void BanManager::add_ban(BanEntry entry) {
    std::lock_guard lock(mutex_);
    auto ipid = entry.ipid;
    bans_[ipid] = std::move(entry);
}

bool BanManager::remove_ban(const std::string& ipid) {
    std::lock_guard lock(mutex_);
    return bans_.erase(ipid) > 0;
}

bool BanManager::is_banned(const std::string& ipid, const std::string& hdid) const {
    std::lock_guard lock(mutex_);

    // Check by IPID
    auto it = bans_.find(ipid);
    if (it != bans_.end() && !it->second.is_expired())
        return true;

    // Check by HDID across all bans
    if (!hdid.empty()) {
        for (auto& [_, entry] : bans_) {
            if (!entry.hdid.empty() && entry.hdid == hdid && !entry.is_expired())
                return true;
        }
    }

    return false;
}

std::optional<BanEntry> BanManager::find_ban(const std::string& ipid) const {
    std::lock_guard lock(mutex_);
    auto it = bans_.find(ipid);
    if (it != bans_.end() && !it->second.is_expired())
        return it->second; // copy
    return std::nullopt;
}

std::optional<BanEntry> BanManager::find_ban_by_hdid(const std::string& hdid) const {
    if (hdid.empty())
        return std::nullopt;
    std::lock_guard lock(mutex_);
    for (auto& [_, entry] : bans_) {
        if (!entry.hdid.empty() && entry.hdid == hdid && !entry.is_expired())
            return entry; // copy
    }
    return std::nullopt;
}

std::optional<BanEntry> BanManager::check_ban(const std::string& ipid, const std::string& hdid) const {
    std::lock_guard lock(mutex_);

    // Check by IPID first
    auto it = bans_.find(ipid);
    if (it != bans_.end() && !it->second.is_expired())
        return it->second;

    // Check by HDID
    if (!hdid.empty()) {
        for (auto& [_, entry] : bans_) {
            if (!entry.hdid.empty() && entry.hdid == hdid && !entry.is_expired())
                return entry;
        }
    }

    return std::nullopt;
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

void BanManager::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        Log::log_print(INFO, "BanManager: no ban file at %s, starting fresh", path.c_str());
        return;
    }

    try {
        nlohmann::json j;
        file >> j;

        if (!j.is_array()) {
            Log::log_print(WARNING, "BanManager: %s is not a JSON array", path.c_str());
            return;
        }

        std::lock_guard lock(mutex_);
        bans_.clear();
        int loaded = 0;
        int skipped = 0;

        for (auto& item : j) {
            BanEntry entry;
            entry.ipid = item.value("ipid", "");
            entry.hdid = item.value("hdid", "");
            entry.reason = item.value("reason", "");
            entry.moderator = item.value("moderator", "");
            entry.timestamp = item.value("timestamp", int64_t(0));
            entry.duration = item.value("duration", int64_t(0));

            if (entry.ipid.empty())
                continue;
            if (entry.is_expired()) {
                ++skipped;
                continue;
            }

            bans_[entry.ipid] = std::move(entry);
            ++loaded;
        }

        Log::log_print(INFO, "BanManager: loaded %d bans from %s (%d expired, skipped)", loaded, path.c_str(), skipped);
    }
    catch (const std::exception& e) {
        Log::log_print(WARNING, "BanManager: failed to parse %s: %s", path.c_str(), e.what());
    }
}

std::unordered_map<std::string, BanEntry> BanManager::snapshot() const {
    std::lock_guard lock(mutex_);
    return bans_;
}

void BanManager::write_to_disk(const std::unordered_map<std::string, BanEntry>& bans, const std::string& path) {
    try {
        nlohmann::json j = nlohmann::json::array();
        for (auto& [_, entry] : bans) {
            if (entry.is_expired())
                continue;
            j.push_back({
                {"ipid", entry.ipid},
                {"hdid", entry.hdid},
                {"reason", entry.reason},
                {"moderator", entry.moderator},
                {"timestamp", entry.timestamp},
                {"duration", entry.duration},
            });
        }

        std::ofstream file(path);
        if (file.is_open()) {
            file << j.dump(2) << std::endl;
            Log::log_print(DEBUG, "BanManager: saved %zu bans to %s", j.size(), path.c_str());
        }
        else {
            Log::log_print(WARNING, "BanManager: failed to open %s for writing", path.c_str());
        }
    }
    catch (const std::exception& e) {
        Log::log_print(WARNING, "BanManager: save failed: %s", e.what());
    }
}

void BanManager::save_async(const std::string& path) {
    {
        std::lock_guard lock(writer_mutex_);
        writer_path_ = path;
        writer_snapshot_ = snapshot();
        writer_pending_ = true;
    }
    writer_cv_.notify_one();
}

void BanManager::save_sync(const std::string& path) const {
    write_to_disk(snapshot(), path);
}

void BanManager::writer_loop(std::stop_token stop) {
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
