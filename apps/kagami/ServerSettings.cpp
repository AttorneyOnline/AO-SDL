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

/// Recursively fill missing keys from @p defaults into @p target.
/// Existing keys in @p target are never overwritten. For nested objects,
/// the merge recurses so that new sub-keys are added without clobbering
/// sibling keys the operator may have edited on disk.
static void fill_missing(nlohmann::json& target, const nlohmann::json& defaults) {
    for (auto& [key, value] : defaults.items()) {
        auto it = target.find(key);
        if (it == target.end()) {
            // Key absent on disk — add the default.
            target[key] = value;
        }
        else if (it->is_object() && value.is_object()) {
            // Both sides are objects — recurse to add missing sub-keys
            // without touching existing ones.
            fill_missing(*it, value);
        }
        // else: key exists on disk — leave it alone.
    }
}

bool ServerSettings::save_to_disk(const std::string& path) {
    // Read the file as it exists RIGHT NOW on disk. This is the
    // authoritative copy — the operator may have edited it directly
    // or via /reload since the server started.
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

    // Build the full default set (defaults + any in-memory overrides).
    auto data = instance().serialize();
    auto ours = nlohmann::json::parse(data, nullptr, false);
    if (!ours.is_object())
        return false;

    // Fill-only merge: add keys the disk file is missing (e.g. new
    // defaults from a binary upgrade) without overwriting anything
    // the operator has set. The file on disk is the source of truth.
    fill_missing(on_disk, ours);

    auto s = on_disk.dump(4) + "\n";

    // Try atomic write (temp + rename). Falls back to direct write if rename
    // fails — e.g. on Docker single-file bind mounts where .tmp is on the
    // overlay filesystem and the target is on the host (cross-device EXDEV).
    auto tmp_path = path + ".tmp";
    {
        std::ofstream tmp(tmp_path, std::ios::binary | std::ios::trunc);
        if (tmp.is_open()) {
            tmp.write(s.data(), s.size());
            tmp.close();

            std::error_code ec;
            std::filesystem::rename(tmp_path, path, ec);
            if (!ec) {
                Log::log_print(INFO, "Saved config to %s", path.c_str());
                config_ops_.labels({"save"}).inc();
                return true;
            }
            // Rename failed (likely EXDEV) — clean up and fall through to direct write
            std::filesystem::remove(tmp_path, ec);
        }
    }

    // Fallback: direct write (not atomic, but works on bind mounts)
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        Log::log_print(WARNING, "Could not write config to %s", path.c_str());
        return false;
    }
    file.write(s.data(), s.size());
    Log::log_print(INFO, "Saved config to %s", path.c_str());
    config_ops_.labels({"save"}).inc();
    return true;
}
