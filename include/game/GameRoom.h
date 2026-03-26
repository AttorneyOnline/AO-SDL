/**
 * @file GameRoom.h
 * @brief Protocol-agnostic shared game state.
 *
 * GameRoom owns the authoritative state for the server: the session registry,
 * character roster, area list, and music list. Both AO and NX backends
 * reference this shared state and delegate actions to it.
 *
 * When an action is processed, GameRoom emits events via registered
 * broadcast callbacks. Each protocol backend registers a callback that
 * serializes events into its wire format and sends them to its clients.
 */
#pragma once

#include "game/GameAction.h"
#include "game/ServerSession.h"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class GameRoom {
  public:
    // --- Broadcast delegates ---
    // Each protocol backend registers one of these to receive events
    // and serialize them for its clients.

    using ICBroadcast = std::function<void(const std::string& area, const ICEvent& evt)>;
    using OOCBroadcast = std::function<void(const std::string& area, const OOCEvent& evt)>;
    using CharSelectBroadcast = std::function<void(const CharSelectEvent& evt)>;
    using MusicBroadcast = std::function<void(const std::string& area, const MusicEvent& evt)>;
    using CharsTakenBroadcast = std::function<void(const std::vector<int>& taken)>;

    void add_ic_broadcast(ICBroadcast cb) {
        ic_broadcasts_.push_back(std::move(cb));
    }
    void add_ooc_broadcast(OOCBroadcast cb) {
        ooc_broadcasts_.push_back(std::move(cb));
    }
    void add_char_select_broadcast(CharSelectBroadcast cb) {
        char_select_broadcasts_.push_back(std::move(cb));
    }
    void add_music_broadcast(MusicBroadcast cb) {
        music_broadcasts_.push_back(std::move(cb));
    }
    void add_chars_taken_broadcast(CharsTakenBroadcast cb) {
        chars_taken_broadcasts_.push_back(std::move(cb));
    }

    // --- Session management (unified across protocols) ---

    ServerSession& create_session(uint64_t client_id, const std::string& protocol);
    void destroy_session(uint64_t client_id);
    ServerSession* get_session(uint64_t client_id);
    size_t session_count() const {
        return sessions_.size();
    }

    /// All sessions in a given area.
    std::vector<ServerSession*> sessions_in_area(const std::string& area);

    // --- Game state ---

    std::vector<std::string> characters;
    std::vector<std::string> music;
    std::vector<std::string> areas;
    std::string server_name;
    std::string server_description;
    int max_players = 100;

    /// Per-character taken state. 0 = available, nonzero = taken.
    std::vector<int> char_taken;
    void reset_taken() {
        char_taken.assign(characters.size(), 0);
    }

    // --- Actions (called by protocol backends) ---

    /// Process an IC message. Validates, then broadcasts to area via all delegates.
    void handle_ic(const ICAction& action);

    /// Process an OOC message. Broadcasts to area via all delegates.
    void handle_ooc(const OOCAction& action);

    /// Process a character selection. Updates char_taken, session, broadcasts.
    /// Returns true if the selection was accepted.
    bool handle_char_select(const CharSelectAction& action);

    /// Process a music change. Broadcasts to area via all delegates.
    void handle_music(const MusicAction& action);

  private:
    std::unordered_map<uint64_t, ServerSession> sessions_;
    uint64_t next_session_id_ = 0;

    std::vector<ICBroadcast> ic_broadcasts_;
    std::vector<OOCBroadcast> ooc_broadcasts_;
    std::vector<CharSelectBroadcast> char_select_broadcasts_;
    std::vector<MusicBroadcast> music_broadcasts_;
    std::vector<CharsTakenBroadcast> chars_taken_broadcasts_;
};
