#pragma once

#include <string>
#include <vector>

/// Per-area settings parsed from areas.ini (akashi-compatible).
struct AreaConfig {
    std::string name;
    std::string background = "gs4";
    bool protected_area = false;
    bool iniswap_allowed = true;
    bool bg_locked = false;
    bool blankposting_allowed = true;
    bool force_immediate = false;
    bool toggle_music = true;
    bool shownames_allowed = true;
    bool ignore_bglist = false;
    std::string evidence_mod = "FFA";
};

/// Reads akashi-format content config files (characters.txt, music.json,
/// areas.ini, backgrounds.txt) from a given directory.
///
/// This makes kagami a drop-in replacement for akashi: operators can
/// point it at their existing config/ directory and everything works.
struct ContentConfig {
    std::vector<std::string> characters;
    std::vector<std::string> music;         ///< Ordered: categories + song names interleaved.
    std::vector<std::string> backgrounds;
    std::vector<std::string> area_names;    ///< Display names in area order.
    std::vector<AreaConfig> area_configs;    ///< Parallel to area_names.

    /// Load all content files from `dir`. Returns true if at least
    /// characters and areas were loaded successfully.
    bool load(const std::string& dir);

    /// Load characters from a line-delimited text file.
    static std::vector<std::string> load_lines(const std::string& path);

    /// Load music list from akashi's music.json format.
    static std::vector<std::string> load_music_json(const std::string& path);

    /// Load areas from akashi's areas.ini format.
    /// Returns area configs in section order (sorted by numeric prefix).
    static std::vector<AreaConfig> load_areas_ini(const std::string& path);
};
