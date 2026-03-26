/**
 * @file NXClient.h
 * @brief AONX protocol handler (client side).
 *
 * Implements ProtocolHandler for the v2 REST+WebSocket protocol.
 * REST handles all client-initiated actions (auth, session, messages).
 * WebSocket receives server-to-client broadcast (IC, OOC, state changes).
 */
#pragma once

#include "NXMessage.h"
#include "net/ProtocolHandler.h"

#include <string>
#include <vector>

/// Connection lifecycle for AONX.
enum class NXConnectionState {
    DISCONNECTED,   ///< No active session.
    AUTHENTICATING, ///< Handshake in progress (token exchange).
    CONNECTED,      ///< Session established, receiving broadcasts.
};

/// AONX protocol handler — client side.
///
/// Unlike AOClient which multiplexes everything over a single WebSocket,
/// NXClient splits transport:
///   - REST (via HttpPool) for request/response actions
///   - WebSocket for server-to-client broadcast
///
/// The ProtocolHandler interface maps to the WebSocket leg only.
/// REST calls are initiated directly by the game layer via typed methods.
class NXClient : public ProtocolHandler {
  public:
    NXClient();

    // --- ProtocolHandler (WebSocket leg) ---
    void on_connect() override;
    void on_message(const std::string& msg) override;
    void on_disconnect() override;
    std::vector<std::string> flush_outgoing() override;

    // --- REST actions (called by game/UI layer) ---

    // TODO: These will be fleshed out as the protocol schema solidifies.
    // void authenticate(const std::string& token);
    // void select_character(int char_id);
    // void send_ic_message(const ICMessageParams& params);
    // void send_ooc_message(const std::string& text);

    // --- State ---
    NXConnectionState state() const {
        return state_;
    }
    const std::string& session_token() const {
        return session_token_;
    }

  private:
    void dispatch(const NXMessage& msg);

    NXConnectionState state_ = NXConnectionState::DISCONNECTED;
    std::string session_token_;
    std::string server_url_; ///< Base URL for REST calls.
    std::vector<std::string> outgoing_;
};
