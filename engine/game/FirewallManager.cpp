#include "game/FirewallManager.h"

#include "utils/Log.h"

#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <json.hpp>

#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif

// -- Helpers ------------------------------------------------------------------

int64_t FirewallManager::now_unix() const {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

bool FirewallManager::is_valid_ip(const std::string& ip) {
    if (ip.empty() || ip.size() > 45)
        return false;

    // IPv4: exactly 4 dot-separated octets 0-255
    if (ip.find('.') != std::string::npos) {
        int a, b, c, d;
        char dummy;
        if (sscanf(ip.c_str(), "%d.%d.%d.%d%c", &a, &b, &c, &d, &dummy) != 4)
            return false;
        return a >= 0 && a <= 255 && b >= 0 && b <= 255 && c >= 0 && c <= 255 && d >= 0 && d <= 255;
    }

    // IPv6: hex digits and colons, must contain at least one colon,
    // at most one :: sequence, max 8 groups, max 39 chars
    if (ip.size() > 39)
        return false;

    bool has_colon = false;
    int double_colon_count = 0;
    int consecutive_colons = 0;
    int groups = 0;

    for (size_t i = 0; i < ip.size(); ++i) {
        char c = ip[i];
        if (c == ':') {
            has_colon = true;
            consecutive_colons++;
            if (consecutive_colons == 2)
                double_colon_count++;
            if (consecutive_colons > 2 || double_colon_count > 1)
                return false;
        }
        else if (std::isxdigit(static_cast<unsigned char>(c))) {
            if (consecutive_colons > 0)
                groups++;
            consecutive_colons = 0;
        }
        else {
            return false;
        }
    }

    if (!has_colon)
        return false;
    if (!ip.empty() && ip.back() != ':')
        groups++;
    if (double_colon_count == 0 && groups != 8)
        return false;
    if (groups > 8)
        return false;

    return true;
}

bool FirewallManager::is_valid_cidr(const std::string& cidr) {
    auto slash = cidr.find('/');
    if (slash == std::string::npos || slash == 0 || slash == cidr.size() - 1)
        return false;

    std::string ip_part = cidr.substr(0, slash);
    std::string prefix_str = cidr.substr(slash + 1);

    if (!is_valid_ip(ip_part))
        return false;

    try {
        int prefix = std::stoi(prefix_str);
        // IPv4: 0-32, IPv6: 0-128
        bool is_v6 = ip_part.find(':') != std::string::npos;
        return prefix >= 0 && prefix <= (is_v6 ? 128 : 32);
    }
    catch (...) {
        return false;
    }
}

// -- Construction / destruction -----------------------------------------------

FirewallManager::FirewallManager() : exec_thread_([this](std::stop_token st) { exec_loop(st); }) {
}

FirewallManager::~FirewallManager() {
    // Flush rules on shutdown if configured
    if (enabled_ && config_.cleanup_on_shutdown) {
        Log::log_print(INFO, "FirewallManager: flushing rules on shutdown");
        // Do this synchronously since we're shutting down
        exec_helper("flush", "");
    }

    exec_thread_.request_stop();
    exec_cv_.notify_one();
}

void FirewallManager::configure(const FirewallConfig& config) {
    std::lock_guard lock(mutex_);
    config_ = config;

    if (!config.enabled || config.helper_path.empty()) {
        enabled_ = false;
        return;
    }

    // Verify helper binary exists and is executable
    try {
        auto status = std::filesystem::status(config.helper_path);
        if (!std::filesystem::exists(status)) {
            Log::log_print(WARNING, "FirewallManager: helper not found at %s", config.helper_path.c_str());
            enabled_ = false;
            return;
        }

#ifndef _WIN32
        if (access(config.helper_path.c_str(), X_OK) != 0) {
            Log::log_print(WARNING, "FirewallManager: helper not executable at %s", config.helper_path.c_str());
            enabled_ = false;
            return;
        }
#endif

        enabled_ = true;
        Log::log_print(INFO, "FirewallManager: enabled with helper at %s", config.helper_path.c_str());

        // Initialize nftables table/sets/chain on first configure
        enqueue({"init", ""});
    }
    catch (const std::exception& e) {
        Log::log_print(WARNING, "FirewallManager: error checking helper: %s", e.what());
        enabled_ = false;
    }
}

bool FirewallManager::is_enabled() const {
    return enabled_;
}

// -- Public operations --------------------------------------------------------

bool FirewallManager::block_ip(const std::string& ip, const std::string& reason, int64_t duration_sec) {
    if (!enabled_)
        return false;
    if (!is_valid_ip(ip)) {
        Log::log_print(WARNING, "FirewallManager: invalid IP: %s", ip.c_str());
        return false;
    }

    {
        std::lock_guard lock(mutex_);
        FirewallRule rule;
        rule.target = ip;
        rule.reason = reason;
        rule.installed_at = now_unix();
        rule.expires_at = (duration_sec == DURATION_PERMANENT) ? 0 : (now_unix() + duration_sec);
        rules_[ip] = rule;
    }

    Log::log_print(INFO, "FirewallManager: blocking IP %s — %s", ip.c_str(), reason.c_str());
    enqueue({"add", ip});
    return true;
}

bool FirewallManager::unblock_ip(const std::string& ip) {
    if (!enabled_)
        return false;

    {
        std::lock_guard lock(mutex_);
        if (rules_.erase(ip) == 0)
            return false;
    }

    Log::log_print(INFO, "FirewallManager: unblocking IP %s", ip.c_str());
    enqueue({"remove", ip});
    return true;
}

bool FirewallManager::block_range(const std::string& cidr, const std::string& reason, int64_t duration_sec) {
    if (!enabled_)
        return false;
    if (!is_valid_cidr(cidr)) {
        Log::log_print(WARNING, "FirewallManager: invalid CIDR: %s", cidr.c_str());
        return false;
    }

    {
        std::lock_guard lock(mutex_);
        FirewallRule rule;
        rule.target = cidr;
        rule.reason = reason;
        rule.installed_at = now_unix();
        rule.expires_at = (duration_sec == DURATION_PERMANENT) ? 0 : (now_unix() + duration_sec);
        rules_[cidr] = rule;
    }

    Log::log_print(INFO, "FirewallManager: blocking range %s — %s", cidr.c_str(), reason.c_str());
    enqueue({"add-range", cidr});
    return true;
}

bool FirewallManager::unblock_range(const std::string& cidr) {
    if (!enabled_)
        return false;

    {
        std::lock_guard lock(mutex_);
        if (rules_.erase(cidr) == 0)
            return false;
    }

    Log::log_print(INFO, "FirewallManager: unblocking range %s", cidr.c_str());
    enqueue({"remove-range", cidr});
    return true;
}

size_t FirewallManager::sweep_expired() {
    if (!enabled_)
        return 0;

    auto now = now_unix();
    std::vector<std::string> to_remove;

    {
        std::lock_guard lock(mutex_);
        for (auto& [target, rule] : rules_) {
            if (rule.expires_at > 0 && rule.expires_at <= now)
                to_remove.push_back(target);
        }

        for (auto& target : to_remove)
            rules_.erase(target);
    }

    for (auto& target : to_remove) {
        bool is_range = target.find('/') != std::string::npos;
        Log::log_print(INFO, "FirewallManager: rule expired for %s", target.c_str());
        enqueue({is_range ? "remove-range" : "remove", target});
    }

    return to_remove.size();
}

std::vector<FirewallRule> FirewallManager::list_rules() const {
    std::lock_guard lock(mutex_);
    std::vector<FirewallRule> result;
    result.reserve(rules_.size());
    for (auto& [_, rule] : rules_)
        result.push_back(rule);
    return result;
}

void FirewallManager::flush() {
    {
        std::lock_guard lock(mutex_);
        rules_.clear();
    }
    Log::log_print(INFO, "FirewallManager: flushing all rules");
    enqueue({"flush", ""});
}

// -- Background execution -----------------------------------------------------

void FirewallManager::enqueue(HelperCommand cmd) {
    {
        std::lock_guard lock(exec_mutex_);
        exec_queue_.push_back(std::move(cmd));
    }
    exec_cv_.notify_one();
}

void FirewallManager::exec_loop(std::stop_token stop) {
    while (!stop.stop_requested()) {
        HelperCommand cmd;

        {
            std::unique_lock lock(exec_mutex_);
            exec_cv_.wait(lock, [&] { return !exec_queue_.empty() || stop.stop_requested(); });
            if (stop.stop_requested())
                break;
            cmd = std::move(exec_queue_.front());
            exec_queue_.pop_front();
        }

        exec_helper(cmd.action, cmd.target);
    }

    // Drain remaining commands on shutdown
    std::lock_guard lock(exec_mutex_);
    while (!exec_queue_.empty()) {
        auto cmd = std::move(exec_queue_.front());
        exec_queue_.pop_front();
        exec_helper(cmd.action, cmd.target);
    }
}

bool FirewallManager::exec_helper(const std::string& action, const std::string& target) {
#ifdef _WIN32
    Log::log_print(WARNING, "FirewallManager: not supported on Windows");
    return false;
#else
    if (config_.helper_path.empty())
        return false;

    Log::log_print(INFO, "FirewallManager: exec %s %s %s", config_.helper_path.c_str(), action.c_str(), target.c_str());

    pid_t pid = fork();
    if (pid < 0) {
        Log::log_print(WARNING, "FirewallManager: fork() failed: %s", strerror(errno));
        return false;
    }

    if (pid == 0) {
        // Child process — exec the helper binary
        // Build argv: [helper_path, action, target (optional), NULL]
        if (target.empty()) {
            execl(config_.helper_path.c_str(), config_.helper_path.c_str(), action.c_str(), nullptr);
        }
        else {
            execl(config_.helper_path.c_str(), config_.helper_path.c_str(), action.c_str(), target.c_str(), nullptr);
        }
        // If exec returns, it failed
        _exit(127);
    }

    // Parent — wait for child
    int status = 0;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return true;
    }

    Log::log_print(WARNING, "FirewallManager: helper exited with status %d for %s %s",
                   WIFEXITED(status) ? WEXITSTATUS(status) : -1, action.c_str(), target.c_str());
    return false;
#endif
}

// -- Persistence (crash recovery) ---------------------------------------------

void FirewallManager::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        return;

    try {
        nlohmann::json j;
        file >> j;
        if (!j.is_array())
            return;

        auto now = now_unix();
        std::lock_guard lock(mutex_);
        int loaded = 0;

        for (auto& item : j) {
            FirewallRule rule;
            rule.target = item.value("target", "");
            rule.reason = item.value("reason", "");
            rule.installed_at = item.value("installed_at", int64_t(0));
            rule.expires_at = item.value("expires_at", int64_t(0));

            if (rule.target.empty())
                continue;
            // Skip expired rules
            if (rule.expires_at > 0 && rule.expires_at <= now)
                continue;

            rules_[rule.target] = rule;
            ++loaded;

            // Re-install the rule
            bool is_range = rule.target.find('/') != std::string::npos;
            enqueue({is_range ? "add-range" : "add", rule.target});
        }

        if (loaded > 0)
            Log::log_print(INFO, "FirewallManager: recovered %d rules from %s", loaded, path.c_str());
    }
    catch (const std::exception& e) {
        Log::log_print(WARNING, "FirewallManager: failed to parse %s: %s", path.c_str(), e.what());
    }
}

void FirewallManager::save_sync(const std::string& path) const {
    std::lock_guard lock(mutex_);

    try {
        nlohmann::json j = nlohmann::json::array();
        for (auto& [_, rule] : rules_) {
            j.push_back({
                {"target", rule.target},
                {"reason", rule.reason},
                {"installed_at", rule.installed_at},
                {"expires_at", rule.expires_at},
            });
        }

        std::ofstream file(path);
        if (file.is_open())
            file << j.dump(2) << std::endl;
    }
    catch (const std::exception& e) {
        Log::log_print(WARNING, "FirewallManager: save failed: %s", e.what());
    }
}
