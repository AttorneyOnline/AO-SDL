#include "ServerSettings.h"

#include "metrics/MetricsRegistry.h"
#include "utils/Log.h"

#include <filesystem>
#include <fstream>
#include <vector>

static auto& config_ops_ =
    metrics::MetricsRegistry::instance().counter("kagami_config_ops_total", "Config file operations", {"op"});

bool ServerSettings::load_from_disk(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        Log::log_print(INFO, "No config file at %s — using defaults", path.c_str());
        return false;
    }

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (!instance().deserialize(data)) {
        Log::log_print(WARNING, "Failed to parse %s — using defaults", path.c_str());
        return false;
    }

    Log::log_print(INFO, "Loaded config from %s", path.c_str());
    config_ops_.labels({"load"}).inc();
    return true;
}

bool ServerSettings::save_to_disk(const std::string& path) {
    // Read the existing file first so we preserve keys this binary doesn't
    // know about (e.g., keys added by a newer build or manual edits).
    nlohmann::json on_disk;
    {
        std::ifstream existing(path, std::ios::binary);
        if (existing.is_open()) {
            std::vector<uint8_t> raw((std::istreambuf_iterator<char>(existing)), std::istreambuf_iterator<char>());
            auto parsed = nlohmann::json::parse(raw, nullptr, false);
            if (parsed.is_object())
                on_disk = std::move(parsed);
        }
    }

    // Build our known state: defaults + loaded values
    auto data = instance().serialize();
    auto ours = nlohmann::json::parse(data, nullptr, false);
    if (!ours.is_object())
        return false;

    // Merge: start with what's on disk, overlay our known values.
    // This adds new default keys without removing unknown ones.
    on_disk.update(ours);

    auto s = on_disk.dump(4) + "\n";

    // Atomic write: temp file + rename to avoid partial writes on crash.
    auto tmp_path = path + ".tmp";
    std::ofstream file(tmp_path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        Log::log_print(WARNING, "Could not write config to %s", path.c_str());
        return false;
    }
    file.write(s.data(), s.size());
    file.close();

    std::error_code ec;
    std::filesystem::rename(tmp_path, path, ec);
    if (ec) {
        Log::log_print(WARNING, "Could not rename %s -> %s: %s", tmp_path.c_str(), path.c_str(), ec.message().c_str());
        return false;
    }
    Log::log_print(INFO, "Saved config to %s", path.c_str());
    config_ops_.labels({"save"}).inc();
    return true;
}
