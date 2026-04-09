#include "ContentConfig.h"

#include "utils/Log.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <json.hpp>

namespace fs = std::filesystem;

// -- Helpers ------------------------------------------------------------------

/// Trim leading/trailing whitespace (including \r for Windows line endings).
static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return {};
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// -- Line-delimited files (characters.txt, backgrounds.txt) -------------------

std::vector<std::string> ContentConfig::load_lines(const std::string& path) {
    std::vector<std::string> result;
    std::ifstream file(path);
    if (!file.is_open()) {
        Log::log_print(WARNING, "ContentConfig: could not open %s", path.c_str());
        return result;
    }

    std::string line;
    while (std::getline(file, line)) {
        auto trimmed = trim(line);
        if (!trimmed.empty())
            result.push_back(std::move(trimmed));
    }
    return result;
}

// -- music.json ---------------------------------------------------------------
//
// Akashi format:
// [
//   {
//     "category": "== Prelude ==",
//     "songs": [
//       { "name": "path/to/song.opus", "length": -1 },
//       ...
//     ]
//   },
//   ...
// ]
//
// The server sends an ordered list of category headers + song names to the
// client via the SM packet. Categories are display-only strings (usually
// prefixed with "=="). Songs are filesystem paths relative to the base
// content folder.

std::vector<std::string> ContentConfig::load_music_json(const std::string& path) {
    std::vector<std::string> result;
    std::ifstream file(path);
    if (!file.is_open()) {
        Log::log_print(WARNING, "ContentConfig: could not open %s", path.c_str());
        return result;
    }

    nlohmann::json root;
    try {
        root = nlohmann::json::parse(file);
    }
    catch (const nlohmann::json::parse_error& e) {
        Log::log_print(WARNING, "ContentConfig: failed to parse %s: %s", path.c_str(), e.what());
        return result;
    }

    if (!root.is_array()) {
        Log::log_print(WARNING, "ContentConfig: %s root is not an array", path.c_str());
        return result;
    }

    for (auto& category_obj : root) {
        if (!category_obj.is_object())
            continue;

        // Category header (e.g. "== Prelude ==")
        auto cat_name = category_obj.value("category", "");
        if (!cat_name.empty())
            result.push_back(cat_name);

        // Songs within this category
        if (category_obj.contains("songs") && category_obj["songs"].is_array()) {
            for (auto& song_obj : category_obj["songs"]) {
                if (!song_obj.is_object())
                    continue;
                auto song_name = song_obj.value("name", "");
                if (!song_name.empty())
                    result.push_back(song_name);
            }
        }
    }
    return result;
}

// -- areas.ini ----------------------------------------------------------------
//
// Akashi format (QSettings INI):
//
//   [0:Basement]
//   background=gs4
//   protected_area=true
//   iniswap_allowed=false
//   ...
//
//   [1:Courtroom 1]
//   background=gs4
//   ...
//
// Section headers are "N:AreaName" where N is the sort index.
// We parse this as a minimal INI reader (no need for a full QSettings port).

/// Simple INI parser that returns sections as maps of key=value pairs.
/// Section names are the raw "[...]" content (e.g. "0:Basement").
struct IniSection {
    std::string header; ///< Raw section name (e.g. "0:Basement").
    std::unordered_map<std::string, std::string> values;
};

static std::vector<IniSection> parse_ini(const std::string& path) {
    std::vector<IniSection> sections;
    std::ifstream file(path);
    if (!file.is_open())
        return sections;

    IniSection* current = nullptr;
    std::string line;
    while (std::getline(file, line)) {
        auto trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#')
            continue;

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            sections.push_back({trimmed.substr(1, trimmed.size() - 2), {}});
            current = &sections.back();
            continue;
        }

        if (!current)
            continue;

        auto eq = trimmed.find('=');
        if (eq == std::string::npos)
            continue;

        auto key = trim(trimmed.substr(0, eq));
        auto val = trim(trimmed.substr(eq + 1));
        if (!key.empty())
            current->values[key] = val;
    }
    return sections;
}

static bool ini_bool(const std::unordered_map<std::string, std::string>& m, const std::string& key, bool def) {
    auto it = m.find(key);
    if (it == m.end())
        return def;
    auto& v = it->second;
    return v == "true" || v == "1" || v == "yes";
}

static std::string ini_str(const std::unordered_map<std::string, std::string>& m, const std::string& key,
                           const std::string& def) {
    auto it = m.find(key);
    return it != m.end() ? it->second : def;
}

std::vector<AreaConfig> ContentConfig::load_areas_ini(const std::string& path) {
    auto sections = parse_ini(path);
    if (sections.empty()) {
        Log::log_print(WARNING, "ContentConfig: no areas found in %s", path.c_str());
        return {};
    }

    // Sort by numeric prefix (akashi uses "N:Name" format).
    std::sort(sections.begin(), sections.end(), [](const IniSection& a, const IniSection& b) {
        // Extract the number before the first ':'
        auto num = [](const std::string& s) -> int {
            auto colon = s.find(':');
            if (colon == std::string::npos)
                return 0;
            try {
                return std::stoi(s.substr(0, colon));
            }
            catch (...) {
                return 0;
            }
        };
        return num(a.header) < num(b.header);
    });

    std::vector<AreaConfig> result;
    result.reserve(sections.size());

    for (auto& sec : sections) {
        AreaConfig cfg;

        // Extract area name: strip "N:" prefix
        auto colon = sec.header.find(':');
        cfg.name = (colon != std::string::npos) ? sec.header.substr(colon + 1) : sec.header;

        cfg.background = ini_str(sec.values, "background", "gs4");
        cfg.protected_area = ini_bool(sec.values, "protected_area", false);
        cfg.iniswap_allowed = ini_bool(sec.values, "iniswap_allowed", true);
        cfg.bg_locked = ini_bool(sec.values, "bg_locked", false);
        cfg.blankposting_allowed = ini_bool(sec.values, "blankposting_allowed", true);
        cfg.force_immediate = ini_bool(sec.values, "force_immediate", false);
        cfg.toggle_music = ini_bool(sec.values, "toggle_music", true);
        cfg.shownames_allowed = ini_bool(sec.values, "shownames_allowed", true);
        cfg.ignore_bglist = ini_bool(sec.values, "ignore_bglist", false);
        cfg.evidence_mod = ini_str(sec.values, "evidence_mod", "FFA");

        result.push_back(std::move(cfg));
    }
    return result;
}

// -- Load all ----------------------------------------------------------------

bool ContentConfig::load(const std::string& dir) {
    std::string char_path = (fs::path(dir) / "characters.txt").string();
    std::string music_path = (fs::path(dir) / "music.json").string();
    std::string areas_path = (fs::path(dir) / "areas.ini").string();
    std::string bg_path = (fs::path(dir) / "backgrounds.txt").string();

    characters = load_lines(char_path);
    music = load_music_json(music_path);
    backgrounds = load_lines(bg_path);
    area_configs = load_areas_ini(areas_path);

    // Derive area_names from area_configs
    area_names.clear();
    area_names.reserve(area_configs.size());
    for (auto& ac : area_configs)
        area_names.push_back(ac.name);

    if (characters.empty())
        Log::log_print(WARNING, "ContentConfig: no characters loaded from %s", char_path.c_str());
    if (music.empty())
        Log::log_print(WARNING, "ContentConfig: no music loaded from %s", music_path.c_str());
    if (area_configs.empty())
        Log::log_print(WARNING, "ContentConfig: no areas loaded from %s", areas_path.c_str());
    if (backgrounds.empty())
        Log::log_print(WARNING, "ContentConfig: no backgrounds loaded from %s", bg_path.c_str());

    Log::log_print(INFO, "ContentConfig: %zu characters, %zu music entries, %zu areas, %zu backgrounds",
                   characters.size(), music.size(), area_configs.size(), backgrounds.size());

    return !characters.empty() && !area_configs.empty();
}
