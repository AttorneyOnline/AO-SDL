/**
 * @file ServerSession.h
 * @brief Represents a player connected to the server.
 *
 * ServerSession holds the common state shared across protocol backends.
 * Lifetime is managed by the backend: AO ties it to socket lifetime,
 * NX ties it to a token that survives reconnects.
 */
#pragma once

#include <chrono>
#include <cstdint>
#include <string>

struct ServerSession {
    uint64_t client_id = 0;    ///< WebSocket client ID (transport-level).
    uint64_t session_id = 0;   ///< Unique server-assigned session ID.
    std::string session_token; ///< Auth token (empty for AO legacy clients).

    std::string display_name;    ///< Player display name / showname.
    std::string client_software; ///< Client name (e.g. "AO-SDL", "AO2").
    int character_id = -1;       ///< Selected character (-1 = none).
    std::string area;            ///< Current area/room.

    /// True if the session has completed the handshake and is fully joined.
    bool joined = false;

    /// Protocol backend that owns this session ("ao2" or "aonx").
    std::string protocol;

    /// Last time this session was active (REST endpoint call, SSE heartbeat).
    /// Used for TTL-based expiry of REST sessions.
    std::chrono::steady_clock::time_point last_activity = std::chrono::steady_clock::now();

    void touch() {
        last_activity = std::chrono::steady_clock::now();
    }
};
