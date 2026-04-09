#pragma once

#include "event/EvidenceListEvent.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

/// Persistent courtroom state that survives character changes.
/// Populated by server events (music list, areas, evidence, players, HP).
/// Only reset on disconnect.
struct CourtroomState {
    // Music/area list (from SM/FM/FA packets)
    std::vector<std::string> areas;
    std::vector<std::string> tracks;
    std::vector<int> area_players;
    std::vector<std::string> area_status;
    std::vector<std::string> area_cm;
    std::vector<std::string> area_lock;
    std::string now_playing;

    // Evidence (from LE packet)
    std::vector<EvidenceItem> evidence;

    // Player list (from PR/PU packets)
    struct PlayerInfo {
        std::string name;
        std::string character;
        std::string charname;
        int area_id = -1;
    };
    std::map<int, PlayerInfo> players;

    /// Our own player ID (session_id from ID packet).
    int local_player_id = -1;

    /// Our current area index (-1 = unknown). Updated from PU AREA_ID for our own player.
    int current_area_id = -1;

    // Health bars (from HP packet)
    int def_hp = 0;
    int pro_hp = 0;

    // OOC chat log (from CT packets)
    struct ChatLine {
        std::string sender;
        std::string message;
        bool system = false;
    };
    std::vector<ChatLine> chat_log;

    /// Reset everything (on disconnect).
    void reset() {
        areas.clear();
        tracks.clear();
        area_players.clear();
        area_status.clear();
        area_cm.clear();
        area_lock.clear();
        now_playing.clear();
        evidence.clear();
        players.clear();
        local_player_id = -1;
        current_area_id = -1;
        def_hp = 0;
        pro_hp = 0;
        chat_log.clear();
    }

    /// Singleton — lives for the app lifetime.
    static CourtroomState& instance() {
        static CourtroomState s;
        return s;
    }
};
