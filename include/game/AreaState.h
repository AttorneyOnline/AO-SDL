/**
 * @file AreaState.h
 * @brief Per-area runtime state: background, music, HP bars, timers.
 *
 * Introduced in AONX Phase 3 (#90). These structs hold the mutable
 * playback state for each area. The REST endpoints serialize them
 * directly into the JSON responses for GET /areas and GET /areas/{id}.
 */
#pragma once

#include <string>
#include <unordered_map>

struct AreaBackground {
    std::string name;
    std::string manifest_hash;
    std::string position; ///< "def", "pro", "wit", etc.
};

struct AreaMusic {
    std::string name;
    std::string asset_hash;
    bool looping = false;
    int channel = 0;
};

struct AreaHP {
    int defense = 10;
    int prosecution = 10;
};

struct AreaTimer {
    int value_ms = 0;
    bool running = false;
};

struct AreaState {
    std::string id;   ///< Deterministic hash of area name (opaque to clients).
    std::string name; ///< Display name (e.g. "Courtroom 1").
    std::string path; ///< Hierarchical slug (e.g. "courtroom-1").

    std::string status = "IDLE"; ///< IDLE, CASING, RECESS.
    std::string cm;              ///< Case maker display name (empty if none).
    bool locked = false;

    AreaBackground background;
    AreaMusic music;
    AreaHP hp;
    std::unordered_map<int, AreaTimer> timers;
};
