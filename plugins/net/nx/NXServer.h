#pragma once

#include "game/GameAction.h"
#include "game/GameRoom.h"

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>

class NXServer {
  public:
    explicit NXServer(GameRoom& room);

    GameRoom& room() {
        return room_;
    }

    struct SessionInfo {
        std::string token;
        uint64_t session_id;
        bool moderator;
    };

    /// Create a session for a REST client. @p remote_addr is the raw
    /// client IP seen by the REST listener (post proxy-header
    /// resolution). It is hashed to form the session's IPID, which
    /// matches the AO2 HI-packet derivation and allows cross-protocol
    /// moderation state (heat, mutes) to key by the same identifier.
    SessionInfo create_session(const std::string& hdid, const std::string& client_name,
                               const std::string& client_version, const std::string& remote_addr = "",
                               bool spectator_admin = false);
    void destroy_session(uint64_t client_id);

    /// Generate a durable auth token (different prefix from session tokens).
    static std::string generate_auth_token();

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

    void set_auth_token_ttl_seconds(int ttl) {
        auth_token_ttl_seconds_.store(ttl, std::memory_order_relaxed);
    }
    int auth_token_ttl_seconds() const {
        return auth_token_ttl_seconds_.load(std::memory_order_relaxed);
    }

  private:
    /// Resolve a transport-level client_id to the user_id string exposed to
    /// clients via the API. Returns nullopt if the session is gone.
    std::optional<std::string> resolve_user_id(uint64_t client_id);

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
    std::atomic<int> auth_token_ttl_seconds_ = 30 * 24 * 60 * 60; // 30 days
};
