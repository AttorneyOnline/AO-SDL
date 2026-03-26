/**
 * @file GameAction.h
 * @brief Protocol-agnostic actions and events for the game layer.
 *
 * Actions represent client-initiated requests (IC message, character select, etc).
 * Events represent validated state changes that all connected clients should see.
 *
 * The flow: protocol backend parses wire format → Action → GameRoom validates →
 * Event → both backends serialize to their wire format and broadcast.
 */
#pragma once

#include <cstdint>
#include <string>

/// An IC (in-character) message submission from any protocol.
///
/// Carries all fields needed to reconstruct the message in any wire format.
/// AO2 MS packets have ~30 positional fields; NX uses named JSON keys.
/// This struct is the superset — backends populate what they have.
struct ICAction {
    uint64_t sender_id = 0;

    // Core fields (all protocols)
    std::string character;
    std::string emote;
    std::string message;
    std::string side;
    std::string showname;
    std::string pre_emote;
    int emote_mod = 0;
    int char_id = 0;
    int desk_mod = 0;
    bool flip = false;
    int text_color = 0;
    int objection_mod = 0;
    int evidence_id = 0;
    bool realization = false;
    bool screenshake = false;
    bool additive = false;
    bool immediate = false;

    // SFX
    std::string sfx_name;
    int sfx_delay = 0;
    bool sfx_looping = false;

    // Frame-level effects (AO2 2.8+)
    std::string frame_screenshake;
    std::string frame_realization;
    std::string frame_sfx;
    std::string effects;

    // Pair/positioning (AO2 2.6+)
    int other_charid = -1;
    std::string self_offset;

    // Misc
    std::string blipname;
    bool slide = false;
};

/// An OOC (out-of-character) message submission.
struct OOCAction {
    uint64_t sender_id = 0;
    std::string name;
    std::string message;
};

/// Character selection request.
struct CharSelectAction {
    uint64_t sender_id = 0;
    int character_id = -1;
};

/// Music change request.
struct MusicAction {
    uint64_t sender_id = 0;
    std::string track;
    std::string showname;
    int channel = 0;
    bool looping = false;
};

// --- Events (validated, ready for broadcast) ---

/// IC message event — broadcast to area.
struct ICEvent {
    std::string area;
    ICAction action;
};

/// OOC message event — broadcast to area.
struct OOCEvent {
    std::string area;
    OOCAction action;
};

/// Character selection confirmed — broadcast to all.
struct CharSelectEvent {
    uint64_t client_id;
    int character_id;
    std::string character_name;
};

/// Music change event — broadcast to area.
struct MusicEvent {
    std::string area;
    MusicAction action;
};
