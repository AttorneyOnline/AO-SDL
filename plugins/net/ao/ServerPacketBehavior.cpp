/**
 * @file ServerPacketBehavior.cpp
 * @brief Server-side handle_server() implementations for AO2 packets.
 *
 * This is the server-side counterpart to PacketBehavior.cpp.
 * Each handler processes a client→server packet and generates
 * the appropriate server→client response per the Akashi reference.
 */
#include "PacketTypes.h"

#include "AOServer.h"
#include "utils/Log.h"

// =============================================================================
// Handshake — client identifying itself
// =============================================================================

// Client sends HI#[hardware_id]#%
// Server responds with ID#[player_number]#kagami#[version]#%
void AOPacketHI::handle_server(AOServer& server, ServerSession& session) {
    auto* proto = server.get_protocol_state(session.client_id);
    if (!proto || proto->state != AOProtocolState::CONNECTED) {
        Log::log_print(WARNING, "AO: unexpected HI from client %llu", (unsigned long long)session.client_id);
        return;
    }

    proto->hardware_id = hardware_id;
    Log::log_print(INFO, "AO: client %llu HWID: %s", (unsigned long long)session.client_id, hardware_id.c_str());

    // Respond with server identification
    server.send(session.client_id, AOPacket("ID", {std::to_string(session.session_id), "kagami", "1.0.0"}));

    proto->state = AOProtocolState::IDENTIFIED;
}

// "ID" packet — bidirectional. When received by server, fields are [software, version].
// The factory creates AOPacketIDClient for all "ID" packets; the 2-field variant
// parses software/version into server_software/server_version fields.
void AOPacketIDClient::handle_server(AOServer& server, ServerSession& session) {
    auto* proto = server.get_protocol_state(session.client_id);
    if (!proto || proto->state != AOProtocolState::IDENTIFIED) {
        Log::log_print(WARNING, "AO: unexpected ID from client %llu", (unsigned long long)session.client_id);
        return;
    }

    Log::log_print(INFO, "AO: client %llu identifies as %s %s", (unsigned long long)session.client_id,
                   server_software.c_str(), server_version.c_str());

    auto& gs = server.game_state();

    // PN — player count
    server.send(session.client_id, AOPacket("PN", {std::to_string(server.session_count()),
                                                   std::to_string(gs.max_players), gs.server_description}));

    // FL — feature list
    server.send(session.client_id, AOPacket("FL", gs.features));

    proto->state = AOProtocolState::LOADING;
}

// =============================================================================
// Loading — client requesting server state
// =============================================================================

// Client sends askchaa#%
// Server responds with SI#[char_count]#[evidence_count]#[music+area_count]#%
void AOPacketAskChaa::handle_server(AOServer& server, ServerSession& session) {
    auto& gs = server.game_state();
    int total_music = static_cast<int>(gs.areas.size() + gs.music.size());

    server.send(session.client_id,
                AOPacket("SI", {std::to_string(gs.characters.size()), "0", std::to_string(total_music)}));
}

// Client sends RC#%
// Server responds with SC#[char1]#[char2]#...#%
void AOPacketRC::handle_server(AOServer& server, ServerSession& session) {
    server.send(session.client_id, AOPacket("SC", server.game_state().characters));
}

// Client sends RM#%
// Server responds with SM#[area1]#...#[music1]#...#%
void AOPacketRM::handle_server(AOServer& server, ServerSession& session) {
    auto& gs = server.game_state();

    // SM combines areas then music into a single field list
    std::vector<std::string> combined;
    combined.reserve(gs.areas.size() + gs.music.size());
    combined.insert(combined.end(), gs.areas.begin(), gs.areas.end());
    combined.insert(combined.end(), gs.music.begin(), gs.music.end());

    server.send(session.client_id, AOPacket("SM", combined));
}

// Client sends RD#%
// Server marks client as joined and sends DONE + initial state
void AOPacketRD::handle_server(AOServer& server, ServerSession& session) {
    auto* proto = server.get_protocol_state(session.client_id);
    if (!proto)
        return;

    proto->state = AOProtocolState::JOINED;
    session.joined = true;

    // Default area
    auto& gs = server.game_state();
    if (!gs.areas.empty())
        session.area = gs.areas[0];

    // CharsCheck — character availability
    std::vector<std::string> taken_fields;
    taken_fields.reserve(gs.char_taken.size());
    for (int t : gs.char_taken)
        taken_fields.push_back(t >= 0 ? "-1" : "0");
    server.send(session.client_id, AOPacket("CharsCheck", taken_fields));

    // DONE — handshake complete, client can now select a character
    server.send(session.client_id, AOPacket("DONE", {}));

    // BN — default background
    server.send(session.client_id, AOPacket("BN", {"gs4", "def"}));

    // HP — health bars (default: full)
    server.send(session.client_id, AOPacket("HP", {"1", "10"}));
    server.send(session.client_id, AOPacket("HP", {"2", "10"}));

    // MOTD via OOC system message
    auto motd = gs.server_description;
    if (!motd.empty()) {
        server.send(session.client_id, AOPacket("CT", {gs.server_name, motd, "1"}));
    }

    Log::log_print(INFO, "AO: client %llu joined (area: %s)", (unsigned long long)session.client_id,
                   session.area.c_str());
}

// =============================================================================
// Character selection
// =============================================================================

// Client sends CC#[player_num]#[char_id]#[hdid]#%
// Server responds with PV#[player_id]#CID#[char_id]#%
void AOPacketCC::handle_server(AOServer& server, ServerSession& session) {
    auto* proto = server.get_protocol_state(session.client_id);
    if (!proto || proto->state != AOProtocolState::JOINED) {
        Log::log_print(WARNING, "AO: CC before join from client %llu", (unsigned long long)session.client_id);
        return;
    }

    auto& gs = server.game_state();

    // Parse char_id from fields (field index 1)
    // CC is constructed with (player_num, char_id, hdid) → fields = [player_num, char_id, hdid]
    int requested_char = -1;
    if (fields.size() >= 2) {
        try {
            requested_char = std::stoi(fields[1]);
        }
        catch (...) {
            return;
        }
    }

    // Validate
    if (requested_char < -1 || requested_char >= static_cast<int>(gs.characters.size())) {
        Log::log_print(WARNING, "AO: invalid char_id %d from client %llu", requested_char,
                       (unsigned long long)session.client_id);
        return;
    }

    // Free previous character
    int old_char = session.character_id;
    if (old_char >= 0 && old_char < static_cast<int>(gs.char_taken.size()))
        gs.char_taken[old_char] = -1;

    // Take new character (if not spectator)
    if (requested_char >= 0) {
        if (gs.char_taken[requested_char] >= 0) {
            Log::log_print(INFO, "AO: char %d already taken, rejecting client %llu", requested_char,
                           (unsigned long long)session.client_id);
            return;
        }
        gs.char_taken[requested_char] = static_cast<int>(session.client_id);
    }

    session.character_id = requested_char;
    if (requested_char >= 0 && requested_char < static_cast<int>(gs.characters.size()))
        session.display_name = gs.characters[requested_char];

    // PV — confirm character assignment
    server.send(session.client_id,
                AOPacket("PV", {std::to_string(session.session_id), "CID", std::to_string(requested_char)}));

    // Broadcast updated CharsCheck to all clients in area
    std::vector<std::string> taken_fields;
    taken_fields.reserve(gs.char_taken.size());
    for (int t : gs.char_taken)
        taken_fields.push_back(t >= 0 ? "-1" : "0");
    server.broadcast_all(AOPacket("CharsCheck", taken_fields));

    Log::log_print(INFO, "AO: client %llu selected character %d (%s)", (unsigned long long)session.client_id,
                   requested_char, session.display_name.c_str());
}

// =============================================================================
// In-game messages
// =============================================================================

// Client sent an IC message. Broadcast to area.
void AOPacketMS::handle_server(AOServer& server, ServerSession& session) {
    if (!session.joined)
        return;

    Log::log_print(INFO, "AO: IC from %s in %s: %s", session.display_name.c_str(), session.area.c_str(),
                   message.c_str());

    server.broadcast_to_area(session.area, *this);
}

// Client sent an OOC message. Broadcast to area.
void AOPacketCT::handle_server(AOServer& server, ServerSession& session) {
    if (!session.joined)
        return;

    Log::log_print(INFO, "AO: OOC from %s in %s: %s", sender_name.c_str(), session.area.c_str(), message.c_str());

    server.broadcast_to_area(session.area, *this);
}

// =============================================================================
// Keepalive
// =============================================================================

void AOPacketCH::handle_server(AOServer& server, ServerSession& session) {
    server.send(session.client_id, AOPacket("CHECK", {}));
}
