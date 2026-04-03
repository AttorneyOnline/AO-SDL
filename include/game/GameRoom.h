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

#include "game/AreaState.h"
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

    /// Fire the chars_taken broadcast to notify all backends of availability changes.
    void broadcast_chars_taken() {
        for (auto& cb : chars_taken_broadcasts_)
            cb(char_taken);
    }

    // --- Session management (unified across protocols) ---

    ServerSession& create_session(uint64_t client_id, const std::string& protocol);
    void destroy_session(uint64_t client_id);
    ServerSession* get_session(uint64_t client_id);
    size_t session_count() const {
        return sessions_.size();
    }

    /// Register a session token for O(1) lookup. Call after setting
    /// session.session_token on a newly created session.
    void register_session_token(const std::string& token, uint64_t client_id);

    /// Find a session by its bearer token, or nullptr if not found.
    ServerSession* find_session_by_token(const std::string& token);

    /// Remove REST sessions that have been inactive for longer than ttl_seconds.
    /// Returns the number of expired sessions removed.
    int expire_sessions(int ttl_seconds);

    /// Scan for expired REST sessions without modifying state.
    /// Returns client_ids of sessions that have exceeded the TTL.
    /// Safe to call under a shared (reader) lock.
    std::vector<uint64_t> find_expired_sessions(int ttl_seconds) const;

    /// Invoke a callback for each active session.
    template <typename F>
    void for_each_session(F&& func) const {
        for (auto& [id, session] : sessions_)
            func(session);
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

    // --- Character ID index (Phase 3) ---

    /// Build the char_id ↔ index maps. Call after populating `characters`.
    void build_char_id_index();

    /// Find a character's vector index by its hash ID, or -1 if not found.
    int find_char_index(const std::string& char_id) const;

    /// Return the hash ID for a character at the given index.
    const std::string& char_id_at(int index) const;

    // --- Area state index (Phase 3) ---

    /// Build per-area state structs. Call after populating `areas`.
    void build_area_index();

    /// Find area state by its hash ID, or nullptr.
    AreaState* find_area(const std::string& area_id);

    /// Find area state by display name, or nullptr.
    AreaState* find_area_by_name(const std::string& area_name);

    /// Read-only access to all area states (keyed by area_id).
    const std::unordered_map<std::string, AreaState>& area_states() const {
        return area_states_;
    }

    // --- Actions (called by protocol backends) ---

    /// Process an IC message. Validates, then broadcasts to area via all delegates.
    void handle_ic(const ICAction& action);

    /// Process an IC message to a specific area (used by NX endpoints for explicit area targeting).
    void handle_ic(const ICAction& action, const std::string& target_area);

    /// Process an OOC message. Broadcasts to area via all delegates.
    void handle_ooc(const OOCAction& action);

    /// Process an OOC message to a specific area (used by NX endpoints for explicit area targeting).
    void handle_ooc(const OOCAction& action, const std::string& target_area);

    /// Process a character selection. Updates char_taken, session, broadcasts.
    /// Returns true if the selection was accepted.
    bool handle_char_select(const CharSelectAction& action);

    /// Process a music change. Broadcasts to area via all delegates.
    void handle_music(const MusicAction& action);

  private:
    std::unordered_map<uint64_t, ServerSession> sessions_;
    std::unordered_map<std::string, uint64_t> token_index_; ///< token → client_id for O(1) lookup.
    uint64_t next_session_id_ = 0;

    // Character ID index (Phase 3)
    std::vector<std::string> char_ids_;                     ///< Parallel to `characters`.
    std::unordered_map<std::string, int> char_id_to_index_; ///< char_id hash → index.
    static const std::string empty_char_id_;                ///< Returned for out-of-range index.

    // Area state index (Phase 3)
    std::unordered_map<std::string, AreaState> area_states_;       ///< area_id hash → state.
    std::unordered_map<std::string, std::string> area_name_to_id_; ///< name → area_id.

    std::vector<ICBroadcast> ic_broadcasts_;
    std::vector<OOCBroadcast> ooc_broadcasts_;
    std::vector<CharSelectBroadcast> char_select_broadcasts_;
    std::vector<MusicBroadcast> music_broadcasts_;
    std::vector<CharsTakenBroadcast> chars_taken_broadcasts_;
};
