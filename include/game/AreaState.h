/**
 * @file AreaState.h
 * @brief Per-area runtime state: background, music, HP bars, timers,
 *        evidence, testimony, lock mode, case masters, invites.
 */
#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

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

// -- Evidence -----------------------------------------------------------------

struct EvidenceItem {
    std::string name;
    std::string description;
    std::string image;
};

// -- Testimony ----------------------------------------------------------------

enum class TestimonyState {
    IDLE,      ///< No testimony in progress.
    RECORDING, ///< CM is recording statements (each IC message is appended).
    PLAYBACK,  ///< Cross-examination mode (navigating through statements).
    UPDATE,    ///< Next IC message replaces the current statement.
    ADD,       ///< Next IC message is inserted after current position.
};

struct TestimonyData {
    TestimonyState state = TestimonyState::IDLE;
    std::vector<std::string> statements; ///< Raw IC packet data per statement.
    int current_index = -1;              ///< Playback position (-1 = none).

    void reset() {
        state = TestimonyState::IDLE;
        statements.clear();
        current_index = -1;
    }
};

// -- Area lock mode -----------------------------------------------------------

enum class AreaLockMode {
    FREE,        ///< Anyone can enter and speak.
    LOCKED,      ///< Only invited clients can enter/speak.
    SPECTATABLE, ///< Anyone can enter, only invited can speak IC.
};

// -- AreaState ----------------------------------------------------------------

struct AreaState {
    std::string id;   ///< Deterministic hash of area name (opaque to clients).
    std::string name; ///< Display name (e.g. "Courtroom 1").
    std::string path; ///< Hierarchical slug (e.g. "courtroom-1").

    std::string status = "IDLE"; ///< IDLE, CASING, RECESS, LOOKING-FOR-PLAYERS, GAMING.
    AreaLockMode lock_mode = AreaLockMode::FREE;

    AreaBackground background;
    AreaMusic music;
    AreaHP hp;
    std::unordered_map<int, AreaTimer> timers;

    // Evidence
    std::vector<EvidenceItem> evidence;

    // Testimony
    TestimonyData testimony;

    // Case masters (client IDs that own this area).
    std::set<uint64_t> cm_owners;

    /// Display name of the current CM (for ARUP display).
    std::string cm;

    /// Invited client IDs (for LOCKED/SPECTATABLE areas).
    std::set<uint64_t> invited;

    /// Whether the background is locked from changes.
    bool bg_locked = false;

    // Helpers

    bool is_cm(uint64_t client_id) const {
        return cm_owners.count(client_id) > 0;
    }

    bool is_invited(uint64_t client_id) const {
        return invited.count(client_id) > 0;
    }

    bool can_speak_ic(uint64_t client_id) const {
        if (lock_mode == AreaLockMode::FREE)
            return true;
        return is_cm(client_id) || is_invited(client_id);
    }

    bool can_enter(uint64_t client_id) const {
        if (lock_mode == AreaLockMode::FREE || lock_mode == AreaLockMode::SPECTATABLE)
            return true;
        return is_cm(client_id) || is_invited(client_id);
    }
};
