#include "PacketTypes.h"

#include "AOServer.h"
#include "game/ASNReputationManager.h"
#include "game/AreaState.h"
#include "game/BanManager.h"
#include "game/ClientId.h"
#include "game/CommandRegistry.h"
#include "game/IPReputationService.h"
#include "game/SpamDetector.h"
#include "net/WebSocketServer.h"
#include "utils/Crypto.h"
#include "utils/Log.h"
#include "utils/Version.h"

#include <chrono>

/// Build an LE (evidence list) packet for an area.
static AOPacket build_evidence_packet(const AreaState& area) {
    std::vector<std::string> items;
    items.reserve(area.evidence.size());
    for (auto& ev : area.evidence)
        items.push_back(ev.name + "&" + ev.description + "&" + ev.image);
    return AOPacket("LE", items);
}

// Handshake

void AOPacketHI::handle_server(AOServer& server, ServerSession& session) {
    auto* proto = server.get_protocol_state(session.client_id);
    if (!proto || proto->state != AOProtocolState::CONNECTED) {
        Log::log_print(WARNING, "AO: unexpected HI from %s", format_client_id(session.client_id).c_str());
        server.send(session.client_id,
                    AOPacket("CT", {"Server", "[ERROR] Unexpected packet \"HI\" in current state", "1"}));
        return;
    }

    proto->hardware_id = hardware_id;
    session.hardware_id = hardware_id;

    // Compute IPID from client IP address (SHA-256, first 8 hex chars).
    if (server.ws()) {
        std::string ip = server.ws()->get_client_addr(session.client_id);
        if (!ip.empty()) {
            session.ip_address = ip;
            session.ipid = crypto::sha256(ip).substr(0, 8);
        }
    }

    Log::log_print(INFO, "AO: %s HWID: %s IPID: %s", format_client_id(session.client_id).c_str(), hardware_id.c_str(),
                   session.ipid.c_str());

    // Ban enforcement: check IPID and HDID before allowing connection
    if (auto* bm = server.room().ban_manager()) {
        if (auto ban = bm->check_ban(session.ipid, session.hardware_id)) {
            Log::log_print(INFO, "AO: %s rejected (banned): %s", format_client_id(session.client_id).c_str(),
                           ban->reason.c_str());
            server.send(session.client_id, AOPacket("KB", {ban->reason}));
            if (server.ws())
                server.ws()->close_client(session.client_id);
            return;
        }
    }

    // IP reputation lookup (async — cache hit is synchronous fast path)
    std::string client_ip;
    if (server.ws())
        client_ip = server.ws()->get_client_addr(session.client_id);

    uint32_t client_asn = 0;
    if (auto* rep = server.room().reputation_service()) {
        // Capture non-owning pointers to long-lived objects (outlive the reputation service).
        // Use close_client_deferred (not close_client) to avoid lock ordering issues —
        // this callback runs on the reputation worker thread, not the dispatch thread.
        auto* ws = server.ws();
        auto* asn_mgr_ptr = server.room().asn_reputation();
        auto cached =
            rep->lookup(client_ip, [ws, asn_mgr_ptr, client_id = session.client_id](const IPReputationEntry& entry) {
                if (asn_mgr_ptr && asn_mgr_ptr->check_blocked(entry.asn)) {
                    Log::log_print(INFO, "AO: %s rejected (ASN blocked): AS%u", format_client_id(client_id).c_str(),
                                   entry.asn);
                    if (ws)
                        ws->close_client_deferred(client_id);
                }
            });

        if (cached) {
            client_asn = cached->asn;

            // Check ASN reputation (synchronous — cache hit)
            if (auto* asn_mgr = server.room().asn_reputation()) {
                if (asn_mgr->check_blocked(client_asn)) {
                    Log::log_print(INFO, "AO: %s rejected (ASN blocked): AS%u (%s)",
                                   format_client_id(session.client_id).c_str(), client_asn, cached->as_org.c_str());
                    server.send(session.client_id, AOPacket("KB", {"Connection blocked (network reputation)"}));
                    if (server.ws())
                        server.ws()->close_client(session.client_id);
                    return;
                }
            }
        }
    }

    // Spam detector: register connection (H2 burst, H5 name pattern, H7 HWID reuse)
    if (auto* sd = server.room().spam_detector()) {
        sd->on_connection(session.ipid, client_asn, session.hardware_id, "" /* username not known yet */);
        sd->record_join_time(session.ipid);
    }

    server.send(session.client_id, AOPacket("ID", {std::to_string(session.session_id), "kagami", ao_sdl_version()}));

    proto->state = AOProtocolState::IDENTIFIED;
}

void AOPacketIDClient::handle_server(AOServer& server, ServerSession& session) {
    auto* proto = server.get_protocol_state(session.client_id);
    if (!proto || proto->state != AOProtocolState::IDENTIFIED) {
        Log::log_print(WARNING, "AO: unexpected ID from %s", format_client_id(session.client_id).c_str());
        server.send(session.client_id,
                    AOPacket("CT", {"Server", "[ERROR] Unexpected packet \"ID\" in current state", "1"}));
        return;
    }

    Log::log_print(INFO, "AO: %s identifies as %s %s", format_client_id(session.client_id).c_str(), software.c_str(),
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
    server.room().stats.joined.fetch_add(1, std::memory_order_relaxed);

    auto& room = server.room();

    std::vector<std::string> taken_fields;
    taken_fields.reserve(room.char_taken.size());
    for (int t : room.char_taken)
        taken_fields.push_back(t ? "-1" : "0");
    server.send(session.client_id, AOPacket("CharsCheck", taken_fields));

    server.send(session.client_id, AOPacket("DONE", {}));
    server.send_area_join_info(session.client_id, session.area);

    // Player list: send full snapshot to this client, then announce this player to all
    server.send_player_list_snapshot(session.client_id);
    server.broadcast_player_add(session.session_id);
    int area_idx = room.area_index(session.area);
    if (area_idx >= 0)
        server.broadcast_player_update(session.session_id, 3, std::to_string(area_idx));

    // ARUP: send full area state to new client, then broadcast player count update
    server.send_full_arup(session.client_id);
    server.broadcast_arup(AOServer::ARUP_PLAYERS);

    if (!room.server_description.empty()) {
        server.send(session.client_id, AOPacket("CT", {room.server_name, room.server_description, "1"}));
    }

    Log::log_print(INFO, "AO: %s joined (area: %s)", format_client_id(session.client_id).c_str(), session.area.c_str());
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
    if (server.room().handle_char_select(action)) {
        // Broadcast character name to all clients
        auto& chars = server.room().characters;
        std::string char_name;
        if (requested_char >= 0 && requested_char < static_cast<int>(chars.size()))
            char_name = chars[requested_char];
        server.broadcast_player_update(session.session_id, 1, char_name);
    }
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
        if (i >= fields.size() || fields[i].empty())
            return def;
        try {
            return std::stoi(fields[i]);
        }
        catch (...) {
            return def;
        }
    };
    auto fb = [&](size_t i) -> bool { return i < fields.size() && fields[i] == "1"; };

    ICAction action;
    action.sender_id = session.client_id;

    // All fields read from raw fields[] at client→server indices.
    // Does NOT use constructor-parsed members to avoid coupling to
    // the server→client index layout used by the deserializing constructor.
    action.desk_mod = (f(0) == "chat") ? -1 : fi(0);
    action.pre_emote = f(1);
    action.character = f(2);
    action.emote = f(3);
    action.message = f(4);
    action.side = f(5);
    action.sfx_name = f(6);
    action.emote_mod = fi(7);
    action.char_id = fi(8);
    action.sfx_delay = fi(9);
    action.objection_mod = fi(10);
    action.evidence_id = fi(11);
    action.flip = fb(12);
    action.realization = fb(13);
    action.text_color = fi(14);
    action.showname = f(15);
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

    // Broadcast showname update if changed
    if (!action.showname.empty() && session.display_name != action.showname) {
        server.broadcast_player_update(session.session_id, 2, action.showname);
    }

    server.room().handle_ic(action);
}

void AOPacketCT::handle_server(AOServer& server, ServerSession& session) {
    if (!session.joined)
        return;

    // Check for OOC commands (messages starting with '/')
    if (!message.empty() && message[0] == '/') {
        CommandContext ctx{
            .room = server.room(),
            .session = session,
            .args = {},
            .send_system_message =
                [&server, &session](const std::string& msg) {
                    server.send(session.client_id, AOPacket("CT", {"Server", msg, "1"}));
                },
            .send_system_message_to =
                [&server](uint64_t client_id, const std::string& msg) {
                    server.send(client_id, AOPacket("CT", {"Server", msg, "1"}));
                },
            .send_area_join_info = [&server](uint64_t client_id,
                                             const std::string& area) { server.send_area_join_info(client_id, area); },
            .broadcast_background =
                [&server](const std::string& area, const std::string& bg) {
                    server.send_to_area(area, AOPacket("BN", {bg, "def"}));
                },
            .broadcast_arup =
                [&server](int arup_type) { server.broadcast_arup(static_cast<AOServer::ArupType>(arup_type)); },
        };

        // Set callbacks that reference ctx (must be done after construction
        // to avoid capturing a not-yet-constructed object).
        ctx.disconnect_client = [&ctx](uint64_t client_id) { ctx.deferred_closes.push_back(client_id); };
        ctx.send_kick_message = [&server, &ctx](uint64_t client_id, const std::string& reason) {
            server.send(client_id, AOPacket("KK", {reason}));
            ctx.deferred_closes.push_back(client_id);
        };
        ctx.send_ban_message = [&server, &ctx](uint64_t client_id, const std::string& reason) {
            server.send(client_id, AOPacket("KB", {reason}));
            ctx.deferred_closes.push_back(client_id);
        };

        if (CommandRegistry::instance().try_dispatch(ctx, message)) {
            // Schedule deferred disconnects. We can't call close_client
            // here because we're inside the dispatch lock (with_lock),
            // and close_client fires on_disconnected which also acquires
            // the dispatch lock, causing deadlock. Instead, queue close
            // frames via the thread-safe queue_send — the WS poll thread
            // will process them and fire on_disconnected outside our lock.
            if (server.ws() && !ctx.deferred_closes.empty()) {
                for (auto id : ctx.deferred_closes)
                    server.ws()->close_client_deferred(id);
            }
            return;
        }
    }

    // Spam detection: check OOC message (H1 echo, H3 join-and-spam)
    if (auto* sd = server.room().spam_detector()) {
        // Look up ASN from reputation cache for this session
        uint32_t asn = 0;
        if (auto* rep = server.room().reputation_service()) {
            if (server.ws()) {
                std::string ip = server.ws()->get_client_addr(session.client_id);
                if (auto cached = rep->find_cached(ip))
                    asn = cached->asn;
            }
        }

        auto verdict = sd->check_message(session.ipid, asn, message);
        if (verdict.is_spam) {
            // Suppress the spam message — don't broadcast it
            Log::log_print(INFO, "AO: OOC from %s suppressed [%s]: %s", format_client_id(session.client_id).c_str(),
                           verdict.heuristic.c_str(), verdict.detail.c_str());
            return;
        }
    }

    // Broadcast OOC name update if changed
    if (session.display_name != sender_name) {
        session.display_name = sender_name;
        server.broadcast_player_update(session.session_id, 0, sender_name);
    }

    OOCAction action;
    action.sender_id = session.client_id;
    action.name = sender_name;
    action.message = message;

    server.room().handle_ooc(action);
}

// Health bars

void AOPacketHP::handle_server(AOServer& server, ServerSession& session) {
    if (!session.joined)
        return;

    // Validate side (1=defense, 2=prosecution) and value (0-10)
    if (side < 1 || side > 2 || value < 0 || value > 10)
        return;

    // Update area state
    auto* area = server.room().find_area_by_name(session.area);
    if (area) {
        if (side == 1)
            area->hp.defense = value;
        else
            area->hp.prosecution = value;
    }

    // Broadcast to area
    server.send_to_area(session.area, AOPacket("HP", {std::to_string(side), std::to_string(value)}));
}

// Music / area change

void AOPacketMC::handle_server(AOServer& server, ServerSession& session) {
    if (!session.joined)
        return;

    auto& room = server.room();

    // Check if the name matches an area (area switch)
    for (auto& area_name : room.areas) {
        if (name == area_name) {
            // Check if the area allows entry
            auto* area = room.find_area_by_name(area_name);
            if (area && !area->can_enter(session.client_id)) {
                server.send(session.client_id, AOPacket("CT", {"Server", "Area " + area_name + " is locked.", "1"}));
                return;
            }

            std::string old_area = session.area;
            session.area = area_name;
            Log::log_print(INFO, "AO: %s moved from %s to %s", format_client_id(session.client_id).c_str(),
                           old_area.c_str(), area_name.c_str());

            // Send area-join info (BN, HP, LE) to the client
            server.send_area_join_info(session.client_id, area_name);

            // Broadcast area change to all clients (PU + ARUP player counts)
            int area_idx = room.area_index(area_name);
            if (area_idx >= 0)
                server.broadcast_player_update(session.session_id, 3, std::to_string(area_idx));
            server.broadcast_arup(AOServer::ARUP_PLAYERS);
            return;
        }
    }

    // Otherwise it's a music change
    MusicAction action;
    action.sender_id = session.client_id;
    action.track = name;
    action.showname = showname;
    action.channel = channel;
    action.looping = looping == 1;
    room.handle_music(action);
}

// Mod call

void AOPacketZZ::handle_server(AOServer& server, ServerSession& session) {
    if (!session.joined)
        return;

    std::string caller = session.display_name.empty() ? "(spectator)" : session.display_name;
    std::string msg = "MOD CALL from " + caller + " [" + session.ipid + "]";
    if (!alert_reason.empty())
        msg += ": " + alert_reason;
    msg += " (area: " + session.area + ")";

    Log::log_print(INFO, "AO: %s", msg.c_str());

    // Send alert to all moderators
    server.room().for_each_session([&](ServerSession& s) {
        if (s.moderator)
            server.send(s.client_id, AOPacket("CT", {"Server", msg, "1"}));
    });
}

// Password

void AOPacketPW::handle_server(AOServer& server, ServerSession& session) {
    session.password = password_;
}

// Moderator action (kick/ban via packet)

void AOPacketMA::handle_server(AOServer& server, ServerSession& session) {
    if (!session.joined || !session.moderator)
        return;

    auto& room = server.room();

    // Collect client IDs to disconnect (deferred to avoid deadlock)
    std::vector<uint64_t> to_close;

    if (duration == 0) {
        // Kick
        room.for_each_session([&](ServerSession& s) {
            if (s.ipid == target_ipid) {
                server.send(s.client_id, AOPacket("KK", {reason}));
                to_close.push_back(s.client_id);
            }
        });
        Log::log_print(INFO, "MA kick: %s [%s] kicked IPID %s (%d): %s", session.display_name.c_str(),
                       session.ipid.c_str(), target_ipid.c_str(), static_cast<int>(to_close.size()), reason.c_str());
    }
    else {
        // Ban
        auto* bm = room.ban_manager();
        if (!bm)
            return;

        std::string target_hdid;
        std::string target_ip;
        room.for_each_session([&](ServerSession& s) {
            if (s.ipid == target_ipid) {
                if (!s.hardware_id.empty())
                    target_hdid = s.hardware_id;
                if (!s.ip_address.empty())
                    target_ip = s.ip_address;
            }
        });

        BanEntry entry;
        entry.ipid = target_ipid;
        entry.hdid = target_hdid;
        entry.ip = target_ip;
        entry.reason = reason;
        entry.moderator = session.display_name;
        entry.timestamp =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
        entry.duration = (duration == -1) ? -2 : duration; // -1 from packet = permanent (-2 internal)
        bm->add_ban(entry);

        room.for_each_session([&](ServerSession& s) {
            if (s.ipid == target_ipid) {
                server.send(s.client_id, AOPacket("KB", {reason}));
                to_close.push_back(s.client_id);
            }
        });

        Log::log_print(INFO, "MA ban: %s [%s] banned IPID %s (dur=%d): %s", session.display_name.c_str(),
                       session.ipid.c_str(), target_ipid.c_str(), duration, reason.c_str());
    }

    // Defer disconnects to the WS poll thread (outside dispatch lock)
    if (server.ws()) {
        for (auto id : to_close)
            server.ws()->close_client_deferred(id);
    }
}

// Testimony animations (WT/CE)

void AOPacketRT::handle_server(AOServer& server, ServerSession& session) {
    if (!session.joined || session.is_spectator())
        return;

    // Broadcast to area
    server.send_to_area(session.area, AOPacket("RT", {animation}));
    Log::log_print(INFO, "AO: RT %s from %s in %s", animation.c_str(), format_client_id(session.client_id).c_str(),
                   session.area.c_str());
}

// Evidence: add

void AOPacketPE::handle_server(AOServer& server, ServerSession& session) {
    if (!session.joined)
        return;

    auto* area = server.room().find_area_by_name(session.area);
    if (!area)
        return;

    area->evidence.push_back({ev_name, ev_description, ev_image});
    server.send_to_area(session.area, build_evidence_packet(*area));
    Log::log_print(INFO, "AO: evidence added by %s in %s: %s", format_client_id(session.client_id).c_str(),
                   session.area.c_str(), ev_name.c_str());
}

// Evidence: edit

void AOPacketEE::handle_server(AOServer& server, ServerSession& session) {
    if (!session.joined)
        return;

    auto* area = server.room().find_area_by_name(session.area);
    if (!area)
        return;

    if (ev_id < 0 || ev_id >= static_cast<int>(area->evidence.size()))
        return;

    area->evidence[ev_id] = {ev_name, ev_description, ev_image};
    server.send_to_area(session.area, build_evidence_packet(*area));
}

// Evidence: delete

void AOPacketDE::handle_server(AOServer& server, ServerSession& session) {
    if (!session.joined)
        return;

    auto* area = server.room().find_area_by_name(session.area);
    if (!area)
        return;

    if (ev_id < 0 || ev_id >= static_cast<int>(area->evidence.size()))
        return;

    area->evidence.erase(area->evidence.begin() + ev_id);
    server.send_to_area(session.area, build_evidence_packet(*area));
}

// Case announcement

void AOPacketCASEA::handle_server(AOServer& server, ServerSession& session) {
    if (!session.joined || session.is_spectator())
        return;

    // Build the CASEA broadcast packet
    AOPacket pkt("CASEA", {case_title, need_def ? "1" : "0", need_pro ? "1" : "0", need_judge ? "1" : "0",
                           need_juror ? "1" : "0", need_steno ? "1" : "0", ""});

    // Send to players who have matching casing preferences
    bool needs[5] = {need_def, need_pro, need_judge, need_juror, need_steno};
    server.room().for_each_session([&](ServerSession& s) {
        if (!s.joined || s.casing_preferences.size() < 5)
            return;
        for (int i = 0; i < 5; ++i) {
            if (needs[i] && s.casing_preferences[i]) {
                server.send(s.client_id, pkt);
                break;
            }
        }
    });

    Log::log_print(INFO, "AO: CASEA from %s: %s", format_client_id(session.client_id).c_str(), case_title.c_str());
}

// Set casing preferences

void AOPacketSETCASE::handle_server(AOServer& server, ServerSession& session) {
    if (!session.joined)
        return;

    session.casing_preferences = preferences;
}

// Keepalive

void AOPacketCH::handle_server(AOServer& server, ServerSession& session) {
    server.send(session.client_id, AOPacket("CHECK", {}));
}
