#include "AOServer.h"

#include "PacketFactory.h"
#include "PacketTypes.h"
#include "game/ClientId.h"
#include "metrics/MetricsRegistry.h"
#include "utils/Log.h"

/// Linker anchor for OOC command registrars.
void ao_register_commands();

static auto& ao_errors_ =
    metrics::MetricsRegistry::instance().counter("kagami_ao_errors_total", "AO protocol errors", {"type"});

AOServer::AOServer(GameRoom& room) : room_(room) {
    ao_register_packet_types();
    ao_register_commands();

    room_.add_ic_broadcast([this](const std::string& area, const ICEvent& evt) { broadcast_ic(area, evt); });
    room_.add_ooc_broadcast([this](const std::string& area, const OOCEvent& evt) { broadcast_ooc(area, evt); });
    room_.add_char_select_broadcast([this](const CharSelectEvent& evt) { broadcast_char_select(evt); });
    room_.add_music_broadcast([this](const std::string& area, const MusicEvent& evt) { broadcast_music(area, evt); });
    room_.add_chars_taken_broadcast([this](const std::vector<int>& taken) { broadcast_chars_taken(taken); });
}

void AOServer::set_send_func(SendFunc func) {
    send_func_ = std::move(func);
}

void AOServer::on_client_connected(uint64_t client_id) {
    room_.create_session(client_id, "ao2");
    proto_state_.emplace(client_id, AOProtocolState{});
    Log::log_print(INFO, "AO: %s connected", format_client_id(client_id).c_str());
    send(client_id, AOPacket("decryptor", {"NOENCRYPT"}));
}

void AOServer::on_client_disconnected(uint64_t client_id) {
    room_.destroy_session(client_id);
    proto_state_.erase(client_id);
    Log::log_print(INFO, "AO: %s disconnected", format_client_id(client_id).c_str());
}

void AOServer::on_client_message(uint64_t client_id, const std::string& raw) {
    auto it = proto_state_.find(client_id);
    if (it == proto_state_.end())
        return;

    if (auto* s = room_.get_session(client_id)) {
        s->bytes_received.fetch_add(raw.size(), std::memory_order_relaxed);
        s->packets_received.fetch_add(1, std::memory_order_relaxed);
    }

    auto& proto = it->second;
    proto.incomplete_buf += raw;

    // Guard against memory exhaustion from clients that never send a delimiter.
    // The largest legitimate AO2 packet (MS with all fields) is well under 4KB.
    static constexpr size_t MAX_INCOMPLETE_BUF = 65536;
    if (proto.incomplete_buf.size() > MAX_INCOMPLETE_BUF) {
        ao_errors_.labels({"buffer_overflow"}).inc();
        Log::log_print(WARNING, "AO: buffer overflow (%zu bytes) from %s", proto.incomplete_buf.size(),
                       format_client_id(client_id).c_str());
        send(client_id, AOPacket("CT", {"Server", "[ERROR] Message too large", "1"}));
        proto.incomplete_buf.clear();
        return;
    }

    std::string& buf = proto.incomplete_buf;
    const std::string delim = AOPacket::DELIMITER;

    size_t pos;
    while ((pos = buf.find(delim)) != std::string::npos) {
        std::string packet_str = buf.substr(0, pos + delim.size());
        buf.erase(0, pos + delim.size());

        Log::log_print(VERBOSE, "AO [%s] <<< %s", format_client_id(client_id).c_str(), packet_str.c_str());

        auto packet = AOPacket::deserialize(packet_str);
        if (packet) {
            try {
                dispatch(client_id, *packet);
            }
            catch (const std::exception& e) {
                ao_errors_.labels({"dispatch"}).inc();
                Log::log_print(WARNING, "AO: error handling packet from %s: %s", format_client_id(client_id).c_str(),
                               e.what());
                // Send error to client before disconnect
                send(client_id, AOPacket("CT", {"Server", "[ERROR] " + std::string(e.what()), "1"}));
            }
        }
        else {
            ao_errors_.labels({"parse"}).inc();
            Log::log_print(WARNING, "AO: malformed packet from %s", format_client_id(client_id).c_str());
            send(client_id, AOPacket("CT", {"Server", "[ERROR] Malformed packet — could not parse", "1"}));
        }
    }
}

AOProtocolState* AOServer::get_protocol_state(uint64_t client_id) {
    auto it = proto_state_.find(client_id);
    return it != proto_state_.end() ? &it->second : nullptr;
}

void AOServer::send(uint64_t client_id, const AOPacket& packet) {
    if (!send_func_)
        return;
    auto serialized = packet.serialize();
    Log::log_print(VERBOSE, "AO [%s] >>> %s", format_client_id(client_id).c_str(), serialized.c_str());
    if (auto* s = room_.get_session(client_id)) {
        s->bytes_sent.fetch_add(serialized.size(), std::memory_order_relaxed);
        s->packets_sent.fetch_add(1, std::memory_order_relaxed);
    }
    send_func_(client_id, serialized);
}

void AOServer::send_to_area(const std::string& area, const AOPacket& packet) {
    if (!send_func_)
        return;
    auto serialized = packet.serialize();
    Log::log_print(VERBOSE, "AO [area:%s] >>> %s", area.c_str(), serialized.c_str());
    for (auto* session : room_.sessions_in_area(area)) {
        if (session->protocol == "ao2")
            send_func_(session->client_id, serialized);
    }
}

void AOServer::send_to_all(const AOPacket& packet) {
    if (!send_func_)
        return;
    auto serialized = packet.serialize();
    Log::log_print(VERBOSE, "AO [all] >>> %s", serialized.c_str());
    for (auto& [id, _] : proto_state_)
        send_func_(id, serialized);
}

void AOServer::dispatch(uint64_t client_id, AOPacket& packet) {
    auto* session = room_.get_session(client_id);
    if (!session)
        return;

    if (!PacketFactory::instance().has_packet(packet.get_header())) {
        ao_errors_.labels({"unknown_packet"}).inc();
        auto truncated = packet.get_header().substr(0, 16);
        Log::log_print(WARNING, "AO: unknown packet \"%s\" from %s", truncated.c_str(),
                       format_client_id(client_id).c_str());
        send(client_id, AOPacket("CT", {"Server", "[ERROR] Unknown packet \"" + truncated + "\"", "1"}));
        return;
    }

    packet.handle_server(*this, *session);
}

void AOServer::broadcast_ic(const std::string& area, const ICEvent& evt) {
    auto& a = evt.action;
    // Server→client MS echo format (all 32 positional fields).
    // Pair character fields (16-21) are placeholders until pairing is implemented.
    AOPacket ms("MS", {
                          std::to_string(a.desk_mod),      // 0
                          a.pre_emote,                     // 1
                          a.character,                     // 2
                          a.emote,                         // 3
                          a.message,                       // 4
                          a.side,                          // 5
                          a.sfx_name,                      // 6
                          std::to_string(a.emote_mod),     // 7
                          std::to_string(a.char_id),       // 8
                          std::to_string(a.sfx_delay),     // 9
                          std::to_string(a.objection_mod), // 10
                          std::to_string(a.evidence_id),   // 11
                          a.flip ? "1" : "0",              // 12
                          a.realization ? "1" : "0",       // 13
                          std::to_string(a.text_color),    // 14
                          a.showname,                      // 15
                          std::to_string(a.other_charid),  // 16: pair char_id
                          "",                              // 17: pair name
                          "",                              // 18: pair emote
                          a.self_offset,                   // 19: self offset
                          "",                              // 20: pair offset
                          "0",                             // 21: pair flip
                          a.immediate ? "1" : "0",         // 22
                          a.sfx_looping ? "1" : "0",       // 23
                          a.screenshake ? "1" : "0",       // 24
                          a.frame_screenshake,             // 25
                          a.frame_realization,             // 26
                          a.frame_sfx,                     // 27
                          a.additive ? "1" : "0",          // 28
                          a.effects,                       // 29
                          a.blipname,                      // 30
                          a.slide ? "1" : "0",             // 31
                      });
    send_to_area(area, ms);
}

void AOServer::broadcast_ooc(const std::string& area, const OOCEvent& evt) {
    send_to_area(area, AOPacket("CT", {evt.action.name, evt.action.message, "0"}));
}

void AOServer::broadcast_music(const std::string& area, const MusicEvent& evt) {
    auto& a = evt.action;
    // MC field 1 is the char_id of the sender, looked up from the session.
    auto* session = room_.get_session(a.sender_id);
    int sender_char_id = session ? session->character_id : -1;
    AOPacket mc("MC", {a.track, std::to_string(sender_char_id), a.showname, a.looping ? "1" : "0",
                       std::to_string(a.channel), "0"});
    send_to_area(area, mc);
}

void AOServer::broadcast_char_select(const CharSelectEvent& evt) {
    auto* session = room_.get_session(evt.client_id);
    if (!session)
        return;
    send(evt.client_id, AOPacket("PV", {std::to_string(session->session_id), "CID", std::to_string(evt.character_id)}));
}

void AOServer::send_area_join_info(uint64_t client_id, const std::string& area_name) {
    auto* area = room_.find_area_by_name(area_name);
    if (!area)
        return;

    // Background
    send(client_id, AOPacket("BN", {area->background.name.empty() ? "gs4" : area->background.name,
                                    area->background.position.empty() ? "def" : area->background.position}));

    // HP bars
    send(client_id, AOPacket("HP", {"1", std::to_string(area->hp.defense)}));
    send(client_id, AOPacket("HP", {"2", std::to_string(area->hp.prosecution)}));

    // Evidence list
    if (!area->evidence.empty()) {
        std::vector<std::string> items;
        items.reserve(area->evidence.size());
        for (auto& ev : area->evidence)
            items.push_back(ev.name + "&" + ev.description + "&" + ev.image);
        send(client_id, AOPacket("LE", items));
    }
}

void AOServer::broadcast_chars_taken(const std::vector<int>& taken) {
    std::vector<std::string> fields;
    fields.reserve(taken.size());
    for (int t : taken)
        fields.push_back(t ? "-1" : "0");
    AOPacket pkt("CharsCheck", fields);
    for (auto& [id, _] : proto_state_)
        send(id, pkt);
}
