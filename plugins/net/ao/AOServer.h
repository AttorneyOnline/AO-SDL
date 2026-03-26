/**
 * @file AOServer.h
 * @brief Server-side AO2 protocol handler.
 *
 * Manages multiple AO2 client sessions over WebSocket, handling the
 * legacy #%-delimited packet format. This enables Kagami to serve
 * legacy AO2 clients during the v2 transition period.
 *
 * Uses the same AOPacket serialization/deserialization as AOClient,
 * but drives the server side of the handshake and state machine.
 */
#pragma once

#include "AOPacket.h"
#include "game/ServerSession.h"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

/// AO2-specific protocol state layered on top of ServerSession.
struct AOProtocolState {
    std::string incomplete_buf; ///< Partial packet buffer (AO2 fragmentation).
    std::string hardware_id;    ///< Client HWID from HI packet.
    std::vector<std::string> features;

    enum State { CONNECTED, IDENTIFIED, LOADING, JOINED };
    State state = CONNECTED;
};

/// Shared game state that the AO2 protocol serves to clients.
struct AOGameState {
    std::vector<std::string> characters; ///< Available character names.
    std::vector<std::string> music;      ///< Music track names.
    std::vector<std::string> areas;      ///< Area/room names.
    std::string server_name = "Kagami";
    std::string server_description;
    int max_players = 100;

    /// Feature flags advertised to clients via FL packet.
    std::vector<std::string> features = {
        "noencryption",       "yellowtext",      "flipping", "customobjections", "fastloading", "deskmod",
        "evidence",           "cccc_ic_support", "arup",     "looping_sfx",      "additive",    "effects",
        "expanded_desk_mods",
    };

    /// Per-character taken state. Index matches characters[]. -1 = available.
    std::vector<int> char_taken;

    void reset_taken() {
        char_taken.assign(characters.size(), -1);
    }
};

/// Server-side AO2 protocol handler.
class AOServer {
  public:
    AOServer();

    using SendFunc = std::function<void(uint64_t client_id, const std::string& data)>;

    /// Set the function used to send data to a specific client.
    void set_send_func(SendFunc func);

    /// Mutable access to the shared game state. Configure before starting.
    AOGameState& game_state() {
        return game_state_;
    }
    const AOGameState& game_state() const {
        return game_state_;
    }

    // --- Session lifecycle ---

    void on_client_connected(uint64_t client_id);
    void on_client_disconnected(uint64_t client_id);

    /// Process a raw message from a client. May produce multiple packets
    /// due to AO2's #% delimiter batching.
    void on_client_message(uint64_t client_id, const std::string& raw);

    ServerSession* get_session(uint64_t client_id);
    AOProtocolState* get_protocol_state(uint64_t client_id);
    size_t session_count() const {
        return sessions_.size();
    }

    // --- Sending ---

    /// Send a packet to a specific client.
    void send(uint64_t client_id, const AOPacket& packet);

    /// Send a packet to all clients in a given area.
    void broadcast_to_area(const std::string& area, const AOPacket& packet);

    /// Send a packet to all connected clients.
    void broadcast_all(const AOPacket& packet);

  private:
    struct ClientEntry {
        ServerSession session;
        AOProtocolState proto;
    };

    void dispatch(ClientEntry& entry, const AOPacket& packet);

    std::unordered_map<uint64_t, ClientEntry> sessions_;
    SendFunc send_func_;
    AOGameState game_state_;
    int next_player_number_ = 0;
};
