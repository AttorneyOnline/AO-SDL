/**
 * @file NXServer.h
 * @brief AONX protocol handler (server side).
 *
 * Handles incoming REST requests and manages WebSocket broadcast
 * for connected clients. This is the server-side counterpart to NXClient.
 *
 * REST endpoints handle: auth, session, character selection, IC/OOC
 * submission, moderation, asset queries.
 *
 * WebSocket pushes: IC messages, OOC chat, background/music changes,
 * moderation actions, presence updates.
 */
#pragma once

#include "NXMessage.h"
#include "game/ServerSession.h"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

/// Server-side AONX protocol handler.
///
/// Owns the session table and provides methods for:
///   - Processing REST API requests (routed from httplib handlers)
///   - Broadcasting events to WebSocket clients
///   - Managing session lifecycle
class NXServer {
  public:
    NXServer();

    // --- Session management ---

    /// Create a new session for a WebSocket client.
    /// Returns the session token.
    std::string create_session(uint64_t client_id);

    /// Remove a session (disconnect/kick).
    void destroy_session(uint64_t client_id);

    /// Look up a session by client ID.
    ServerSession* get_session(uint64_t client_id);

    /// Look up a session by token.
    ServerSession* get_session_by_token(const std::string& token);

    /// Number of active sessions.
    size_t session_count() const {
        return sessions_.size();
    }

    // --- Broadcast ---

    using BroadcastFunc = std::function<void(uint64_t client_id, const std::string& data)>;

    /// Set the function used to send data to a specific WebSocket client.
    /// Typically wired to WebSocketServer::send().
    void set_broadcast_func(BroadcastFunc func);

    /// Broadcast a message to all clients in a given area.
    void broadcast_to_area(const std::string& area, const NXMessage& msg);

    /// Broadcast a message to all connected clients.
    void broadcast_all(const NXMessage& msg);

    // --- REST request handling ---

    // TODO: Handler methods for each REST endpoint.
    // These will be called from httplib route handlers in Kagami's main.
    //
    // NXMessage handle_authenticate(const NXMessage& request);
    // NXMessage handle_select_character(const ServerSession& session, const NXMessage& request);
    // NXMessage handle_ic_message(const ServerSession& session, const NXMessage& request);
    // NXMessage handle_ooc_message(const ServerSession& session, const NXMessage& request);
    // NXMessage handle_moderation_action(const ServerSession& session, const NXMessage& request);

  private:
    std::unordered_map<uint64_t, ServerSession> sessions_; ///< client_id → session
    BroadcastFunc broadcast_func_;
};
