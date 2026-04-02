#pragma once

#include "game/GameAction.h"
#include "game/GameRoom.h"

#include <atomic>
#include <cstdint>
#include <string>

class NXServer {
  public:
    explicit NXServer(GameRoom& room);

    GameRoom& room() {
        return room_;
    }

    /// Create a session for a REST client. Returns the bearer token.
    std::string create_session(const std::string& hdid, const std::string& client_name,
                               const std::string& client_version);
    void destroy_session(uint64_t client_id);

    /// Server configuration values, set once at startup by main.cpp
    /// before any HTTP threads are running. Getters are called from
    /// handler threads. The string is immutable after init; the TTL
    /// is atomic for safe reads from handler threads.
    void set_motd(const std::string& motd) {
        motd_ = motd;
    }
    const std::string& motd() const {
        return motd_;
    }

    void set_session_ttl_seconds(int ttl) {
        session_ttl_seconds_.store(ttl, std::memory_order_relaxed);
    }
    int session_ttl_seconds() const {
        return session_ttl_seconds_.load(std::memory_order_relaxed);
    }

  private:
    // Broadcast callbacks registered with GameRoom.
    // Serialize events to JSON and publish as SSEEvents for NX clients.
    void broadcast_ic(const std::string& area, const ICEvent& evt);
    void broadcast_ooc(const std::string& area, const OOCEvent& evt);
    void broadcast_char_select(const CharSelectEvent& evt);
    void broadcast_chars_taken(const std::vector<int>& taken);
    void broadcast_music(const std::string& area, const MusicEvent& evt);

    GameRoom& room_;
    std::atomic<uint64_t> next_rest_id_ =
        0x8000'0000'0000'0000ULL; ///< High bit set to avoid collision with WS client IDs.
    std::string motd_;            ///< Set once before HTTP threads start.
    std::atomic<int> session_ttl_seconds_ = 300;
};
