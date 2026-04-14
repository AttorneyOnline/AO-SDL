/**
 * @file ServerSession.h
 * @brief Represents a player connected to the server.
 *
 * ServerSession holds the common state shared across protocol backends.
 * Lifetime is managed by the backend: AO ties it to socket lifetime,
 * NX ties it to a token that survives reconnects.
 */
#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

struct ServerSession {
    ServerSession() = default;
    ServerSession(ServerSession&& o) noexcept
        : client_id(o.client_id), session_id(o.session_id), session_token(std::move(o.session_token)),
          display_name(std::move(o.display_name)), client_software(std::move(o.client_software)),
          character_id(o.character_id), area(std::move(o.area)), ipid(std::move(o.ipid)),
          ip_address(std::move(o.ip_address)), hardware_id(std::move(o.hardware_id)), password(std::move(o.password)),
          casing_preferences(std::move(o.casing_preferences)), joined(o.joined), moderator(o.moderator),
          acl_role(std::move(o.acl_role)), auth_token_id(std::move(o.auth_token_id)),
          moderator_name(std::move(o.moderator_name)),
          change_auth_started(o.change_auth_started), protocol(std::move(o.protocol)),
          last_activity_ns(o.last_activity_ns.load(std::memory_order_relaxed)),
          bytes_sent(o.bytes_sent.load(std::memory_order_relaxed)),
          bytes_received(o.bytes_received.load(std::memory_order_relaxed)),
          packets_sent(o.packets_sent.load(std::memory_order_relaxed)),
          packets_received(o.packets_received.load(std::memory_order_relaxed)),
          mod_actions(o.mod_actions.load(std::memory_order_relaxed)) {
    }
    ServerSession& operator=(ServerSession&&) = delete;
    ServerSession(const ServerSession&) = delete;
    ServerSession& operator=(const ServerSession&) = delete;

    uint64_t client_id = 0;    ///< WebSocket client ID (transport-level).
    uint64_t session_id = 0;   ///< Unique server-assigned session ID.
    std::string session_token; ///< Auth token (empty for AO legacy clients).

    std::string display_name;    ///< Player display name / showname.
    std::string client_software; ///< Client name (e.g. "AO-SDL", "AO2").
    int character_id = -1;       ///< Selected character (-1 = none).
    std::string area;            ///< Current area/room.

    std::string ipid;        ///< SHA-256(IP).left(8), stable short identifier for moderation.
    std::string ip_address;  ///< Raw client IP (for telemetry/admin). Not sent to other clients.
    std::string hardware_id; ///< Hardware ID reported by the client (HDID).

    /// Password sent via PW packet (used in some auth flows).
    std::string password;

    /// Casing preferences for CASEA announcements.
    /// 5 booleans: def_attorney, prosecutor, judge, juror, stenographer.
    std::vector<bool> casing_preferences;

    /// True if the session has completed the handshake and is fully joined.
    bool joined = false;

    /// True if the session has moderator privileges (e.g. all-areas broadcast).
    /// In ADVANCED auth mode, this is set when the user has any non-NONE ACL role.
    bool moderator = false;

    /// ACL role identifier (e.g. "SUPER", "MOD", "NONE"). Empty if not authenticated.
    std::string acl_role;

    /// Auth token that created this session (for active kill on token revocation).
    /// Empty for anonymous sessions and AO2 clients that authenticated via OOC /login.
    std::string auth_token_id;

    /// Username from database auth (ADVANCED mode only). Empty in SIMPLE mode.
    std::string moderator_name;

    /// Transient flag: true while the /changeauth flow is in progress.
    bool change_auth_started = false;

    /// Failed login attempt count (for rate-limiting brute force).
    int login_failures = 0;

    /// Steady clock time of last failed login (for cooldown).
    std::chrono::steady_clock::time_point last_login_failure{};

    /// True if the client is a spectator (character_id == -1 after joining).
    bool is_spectator() const {
        return character_id < 0;
    }

    /// Protocol backend that owns this session ("ao2" or "aonx").
    std::string protocol;

    /// Last time this session was active (REST endpoint call, SSE heartbeat).
    /// Used for TTL-based expiry of REST sessions. Stored as nanoseconds since
    /// steady_clock epoch so it can be atomic — allows touch() under a shared
    /// (reader) lock without exclusive access.
    std::atomic<int64_t> last_activity_ns{std::chrono::steady_clock::now().time_since_epoch().count()};

    void touch() {
        last_activity_ns.store(std::chrono::steady_clock::now().time_since_epoch().count(), std::memory_order_relaxed);
    }

    /// Read last_activity as a time_point (convenience for TTL checks).
    std::chrono::steady_clock::time_point last_activity() const {
        return std::chrono::steady_clock::time_point{
            std::chrono::steady_clock::duration{last_activity_ns.load(std::memory_order_relaxed)}};
    }

    // -- Per-session traffic counters (atomic, updated inline at I/O sites) ---
    std::atomic<uint64_t> bytes_sent{0};
    std::atomic<uint64_t> bytes_received{0};
    std::atomic<uint64_t> packets_sent{0};
    std::atomic<uint64_t> packets_received{0};
    std::atomic<uint64_t> mod_actions{0};
};
