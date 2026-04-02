#include "NXServer.h"

#include "event/EventManager.h"
#include "game/ClientId.h"
#include "net/SSEEvent.h"
#include "utils/Crypto.h"
#include "utils/Log.h"

#include <json.hpp>

#include <cstdio>
#include <random>

// -- Token generation --------------------------------------------------------
// Tokens are SHA-256("kagamitok:{server_iv}:{csprng_random}").
// The server IV is a 64-bit value generated once at startup from the system
// CSPRNG, providing per-instance uniqueness. The random component is 128 bits
// of CSPRNG output per token.

static std::string generate_server_iv() {
    std::random_device rd;
    uint32_t hi = rd();
    uint32_t lo = rd();
    char buf[17];
    std::snprintf(buf, sizeof(buf), "%08x%08x", hi, lo);
    return buf;
}

static const std::string& server_iv() {
    static const std::string iv = generate_server_iv();
    return iv;
}

static std::string generate_token() {
    std::random_device rd;
    char rand_hex[33];
    std::snprintf(rand_hex, sizeof(rand_hex), "%08x%08x%08x%08x", rd(), rd(), rd(), rd());

    return crypto::sha256("kagamitok:" + server_iv() + ":" + rand_hex);
}

// -- NXServer ----------------------------------------------------------------

NXServer::NXServer(GameRoom& room) : room_(room) {
    room_.add_ic_broadcast([this](const std::string& area, const ICEvent& evt) { broadcast_ic(area, evt); });
    room_.add_ooc_broadcast([this](const std::string& area, const OOCEvent& evt) { broadcast_ooc(area, evt); });
    room_.add_char_select_broadcast([this](const CharSelectEvent& evt) { broadcast_char_select(evt); });
    room_.add_chars_taken_broadcast([this](const std::vector<int>& taken) { broadcast_chars_taken(taken); });
    room_.add_music_broadcast([this](const std::string& area, const MusicEvent& evt) { broadcast_music(area, evt); });
}

std::string NXServer::create_session(const std::string& hdid, const std::string& client_name,
                                     const std::string& client_version) {
    // TODO: store hdid on session for future ban/rate-limiting support
    (void)hdid;
    uint64_t id = next_rest_id_++;
    auto& session = room_.create_session(id, "aonx");
    session.session_token = generate_token();
    room_.register_session_token(session.session_token, id);
    session.display_name = client_name;
    session.client_software = client_name + "/" + client_version;
    session.joined = true;
    Log::log_print(INFO, "NX: session created (%s, client=%s)", format_client_id(id).c_str(),
                   session.client_software.c_str());
    return session.session_token;
}

void NXServer::destroy_session(uint64_t client_id) {
    room_.destroy_session(client_id);
    Log::log_print(INFO, "NX: session destroyed for %s", format_client_id(client_id).c_str());
}

// -- Broadcast → SSE event publishing -----------------------------------------

static void publish_sse(const std::string& event_type, nlohmann::json payload, const std::string& area) {
    SSEEvent sse;
    sse.event = event_type;
    sse.data = payload.dump();
    sse.area = area;
    EventManager::instance().get_channel<SSEEvent>().publish(std::move(sse));
}

void NXServer::broadcast_ic(const std::string& area, const ICEvent& evt) {
    auto& a = evt.action;
    nlohmann::json j;
    j["character"] = a.character;
    j["message"] = a.message;
    j["showname"] = a.showname;
    j["emote"] = a.emote;
    j["side"] = a.side;
    j["pre_emote"] = a.pre_emote;
    j["emote_mod"] = a.emote_mod;
    j["char_id"] = a.char_id;
    j["desk_mod"] = a.desk_mod;
    j["flip"] = a.flip;
    j["text_color"] = a.text_color;
    j["objection_mod"] = a.objection_mod;
    j["evidence_id"] = a.evidence_id;
    j["realization"] = a.realization;
    j["screenshake"] = a.screenshake;
    j["additive"] = a.additive;
    j["immediate"] = a.immediate;
    if (!a.sfx_name.empty()) {
        j["sfx_name"] = a.sfx_name;
        j["sfx_delay"] = a.sfx_delay;
        j["sfx_looping"] = a.sfx_looping;
    }
    if (!a.effects.empty())
        j["effects"] = a.effects;
    if (!a.blipname.empty())
        j["blipname"] = a.blipname;
    publish_sse("ic_message", std::move(j), area);
}

void NXServer::broadcast_ooc(const std::string& area, const OOCEvent& evt) {
    publish_sse("ooc_message", {{"name", evt.action.name}, {"message", evt.action.message}}, area);
}

void NXServer::broadcast_char_select(const CharSelectEvent& evt) {
    publish_sse(
        "char_taken",
        {{"client_id", evt.client_id}, {"character_id", evt.character_id}, {"character_name", evt.character_name}},
        ""); // global broadcast
}

void NXServer::broadcast_chars_taken(const std::vector<int>& taken) {
    publish_sse("char_taken", {{"taken", taken}}, ""); // global broadcast
}

void NXServer::broadcast_music(const std::string& area, const MusicEvent& evt) {
    publish_sse("music_change",
                {{"track", evt.action.track},
                 {"showname", evt.action.showname},
                 {"channel", evt.action.channel},
                 {"looping", evt.action.looping}},
                area);
}
