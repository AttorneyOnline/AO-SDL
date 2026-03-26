#include "PacketTypes.h"

#include "AOServer.h"
#include "utils/Log.h"

// Handshake

void AOPacketHI::handle_server(AOServer& server, ServerSession& session) {
    auto* proto = server.get_protocol_state(session.client_id);
    if (!proto || proto->state != AOProtocolState::CONNECTED) {
        Log::log_print(WARNING, "AO: unexpected HI from client %llu", (unsigned long long)session.client_id);
        return;
    }

    proto->hardware_id = hardware_id;
    Log::log_print(INFO, "AO: client %llu HWID: %s", (unsigned long long)session.client_id, hardware_id.c_str());

    server.send(session.client_id, AOPacket("ID", {std::to_string(session.session_id), "kagami", "1.0.0"}));

    proto->state = AOProtocolState::IDENTIFIED;
}

void AOPacketIDClient::handle_server(AOServer& server, ServerSession& session) {
    auto* proto = server.get_protocol_state(session.client_id);
    if (!proto || proto->state != AOProtocolState::IDENTIFIED) {
        Log::log_print(WARNING, "AO: unexpected ID from client %llu", (unsigned long long)session.client_id);
        return;
    }

    Log::log_print(INFO, "AO: client %llu identifies as %s %s", (unsigned long long)session.client_id, software.c_str(),
                   version.c_str());

    auto& room = server.room();

    server.send(session.client_id, AOPacket("PN", {std::to_string(room.session_count()),
                                                   std::to_string(room.max_players), room.server_description}));

    std::vector<std::string> features = {
        "noencryption",       "yellowtext",      "flipping", "customobjections", "fastloading", "deskmod",
        "evidence",           "cccc_ic_support", "arup",     "looping_sfx",      "additive",    "effects",
        "expanded_desk_mods",
    };
    server.send(session.client_id, AOPacket("FL", features));

    proto->state = AOProtocolState::LOADING;
}

// Loading

void AOPacketAskChaa::handle_server(AOServer& server, ServerSession& session) {
    auto& room = server.room();
    int total_music = static_cast<int>(room.areas.size() + room.music.size());

    server.send(session.client_id,
                AOPacket("SI", {std::to_string(room.characters.size()), "0", std::to_string(total_music)}));
}

void AOPacketRC::handle_server(AOServer& server, ServerSession& session) {
    server.send(session.client_id, AOPacket("SC", server.room().characters));
}

void AOPacketRM::handle_server(AOServer& server, ServerSession& session) {
    auto& room = server.room();
    std::vector<std::string> combined;
    combined.reserve(room.areas.size() + room.music.size());
    combined.insert(combined.end(), room.areas.begin(), room.areas.end());
    combined.insert(combined.end(), room.music.begin(), room.music.end());
    server.send(session.client_id, AOPacket("SM", combined));
}

void AOPacketRD::handle_server(AOServer& server, ServerSession& session) {
    auto* proto = server.get_protocol_state(session.client_id);
    if (!proto)
        return;

    proto->state = AOProtocolState::JOINED;
    session.joined = true;

    auto& room = server.room();

    std::vector<std::string> taken_fields;
    taken_fields.reserve(room.char_taken.size());
    for (int t : room.char_taken)
        taken_fields.push_back(t ? "-1" : "0");
    server.send(session.client_id, AOPacket("CharsCheck", taken_fields));

    server.send(session.client_id, AOPacket("DONE", {}));
    server.send(session.client_id, AOPacket("BN", {"gs4", "def"}));
    server.send(session.client_id, AOPacket("HP", {"1", "10"}));
    server.send(session.client_id, AOPacket("HP", {"2", "10"}));

    if (!room.server_description.empty()) {
        server.send(session.client_id, AOPacket("CT", {room.server_name, room.server_description, "1"}));
    }

    Log::log_print(INFO, "AO: client %llu joined (area: %s)", (unsigned long long)session.client_id,
                   session.area.c_str());
}

// Actions — delegated to GameRoom

void AOPacketCC::handle_server(AOServer& server, ServerSession& session) {
    if (!session.joined)
        return;

    int requested_char = -1;
    if (fields.size() >= 2) {
        try {
            requested_char = std::stoi(fields[1]);
        }
        catch (...) {
            return;
        }
    }

    CharSelectAction action;
    action.sender_id = session.client_id;
    action.character_id = requested_char;
    server.room().handle_char_select(action);
}

void AOPacketMS::handle_server(AOServer& server, ServerSession& session) {
    if (!session.joined)
        return;

    // Parse from raw fields using CLIENT→SERVER indices.
    // The constructor parses at server→client indices which are wrong for
    // extension fields (pair character fields 16-21 don't exist in client packets).
    //
    // Client→server MS layout:
    //   0: desk_mod, 1: pre_emote, 2: character, 3: emote, 4: message,
    //   5: side, 6: sfx_name, 7: emote_mod, 8: char_id, 9: sfx_delay,
    //   10: objection_mod, 11: evidence_id, 12: flip, 13: realization,
    //   14: text_color, 15: showname, 16: other_charid, 17: self_offset,
    //   18: immediate, 19: looping_sfx, 20: screenshake,
    //   21: frame_screenshake, 22: frame_realization, 23: frame_sfx,
    //   24: additive, 25: effects, 26: blipname, 27: slide

    auto f = [this](size_t i) -> const std::string& {
        static const std::string empty;
        return i < fields.size() ? fields[i] : empty;
    };
    auto fi = [&](size_t i, int def = 0) -> int {
        if (i >= fields.size() || fields[i].empty()) return def;
        try { return std::stoi(fields[i]); } catch (...) { return def; }
    };
    auto fb = [&](size_t i) -> bool {
        return i < fields.size() && fields[i] == "1";
    };

    ICAction action;
    action.sender_id = session.client_id;

    // 0-14: same layout in both formats (constructor parsed these correctly)
    action.desk_mod = desk_mod;
    action.pre_emote = pre_emote;
    action.character = character;
    action.emote = emote;
    action.message = message;
    action.side = side;
    action.sfx_name = sfx_name;
    action.emote_mod = emote_mod;
    action.char_id = char_id;
    action.sfx_delay = sfx_delay;
    action.objection_mod = objection_mod;
    action.evidence_id = fi(11); // 0 = no evidence, 1+ = evidence list index
    action.flip = flip;
    action.realization = realization;
    action.text_color = text_color;

    // 15+: client→server indices (NO pair fields — those are server-injected on echo)
    action.showname = showname; // 15 — same in both formats
    action.other_charid = fi(16, -1);
    action.self_offset = f(17);
    action.immediate = fb(18);
    action.sfx_looping = fb(19);
    action.screenshake = fb(20);
    action.frame_screenshake = f(21);
    action.frame_realization = f(22);
    action.frame_sfx = f(23);
    action.additive = fb(24);
    action.effects = f(25);
    action.blipname = f(26);
    action.slide = fb(27);

    server.room().handle_ic(action);
}

void AOPacketCT::handle_server(AOServer& server, ServerSession& session) {
    if (!session.joined)
        return;

    OOCAction action;
    action.sender_id = session.client_id;
    action.name = sender_name;
    action.message = message;

    server.room().handle_ooc(action);
}

// Keepalive

void AOPacketCH::handle_server(AOServer& server, ServerSession& session) {
    server.send(session.client_id, AOPacket("CHECK", {}));
}
