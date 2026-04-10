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

NXServer::SessionInfo NXServer::create_session(const std::string& hdid, const std::string& client_name,
                                               const std::string& client_version,
                                               const std::string& remote_addr) {
    uint64_t id = next_rest_id_++;
    auto token = generate_token();

    auto session = room_.create_session_with_token(id, "aonx", token);
    session->display_name = client_name;
    session->client_software = client_name + "/" + client_version;
    session->hardware_id = hdid;
    session->ip_address = remote_addr;
    // Match the AO2 HI-packet derivation: SHA-256 of the client IP,
    // first 8 hex chars. This gives cross-protocol moderation state
    // (heat, mutes) a stable key regardless of which backend the
    // client connected through.
    if (!remote_addr.empty())
        session->ipid = crypto::sha256(remote_addr).substr(0, 8);
    session->joined = true;
    room_.stats.joined.fetch_add(1, std::memory_order_relaxed);

    Log::log_print(INFO, "NX: session created (%s, client=%s, ipid=%s)", format_client_id(id).c_str(),
                   session->client_software.c_str(), session->ipid.c_str());
    return {session->session_token, session->session_id, session->moderator};
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
    Log::log_print(VERBOSE, "SSE: publish %s area=%s %s", event_type.c_str(), area.empty() ? "*" : area.c_str(),
                   sse.data.c_str());
    EventManager::instance().get_channel<SSEEvent>().publish(std::move(sse));
}

/// Resolve a client_id (transport-level) to the user_id string exposed to
/// clients. Returns nullopt if the session is not found.
std::optional<std::string> NXServer::resolve_user_id(uint64_t client_id) {
    if (auto* s = room_.get_session(client_id))
        return std::to_string(s->session_id);
    return std::nullopt;
}

/// Set "user_id" on a JSON object if the resolved value is present.
static void set_user_id(nlohmann::json& j, const std::optional<std::string>& uid) {
    if (uid)
        j["user_id"] = *uid;
}

void NXServer::broadcast_ic(const std::string& area, const ICEvent& evt) {
    auto& a = evt.action;
    nlohmann::json j;
    set_user_id(j, resolve_user_id(a.sender_id));
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
    nlohmann::json j = {{"name", evt.action.name}, {"message", evt.action.message}};
    set_user_id(j, resolve_user_id(evt.action.sender_id));
    publish_sse("ooc_message", std::move(j), area);
}

void NXServer::broadcast_char_select(const CharSelectEvent& evt) {
    // Only emit per-character char_taken when a character was actually selected.
    // Deselection (character_id = -1) is covered by broadcast_chars_taken which
    // sends the full availability array.
    if (evt.character_id < 0)
        return;
    // TODO: char_id should be the SHA-256 manifest hash per the spec, not the
    // integer index. CharSelectEvent only carries the legacy index; resolving
    // the hash requires the character manifest store (Phase 3).
    nlohmann::json j = {
        {"char_id", std::to_string(evt.character_id)},
        {"available", false},
    };
    set_user_id(j, resolve_user_id(evt.client_id));
    publish_sse("char_taken", std::move(j), ""); // global broadcast
}

void NXServer::broadcast_chars_taken(const std::vector<int>& taken) {
    publish_sse("char_taken", {{"taken", taken}}, ""); // global broadcast
}

void NXServer::broadcast_music(const std::string& area, const MusicEvent& evt) {
    nlohmann::json j = {
        {"name", evt.action.track},
        {"showname", evt.action.showname},
        {"channel", evt.action.channel},
        {"looping", evt.action.looping},
    };
    set_user_id(j, resolve_user_id(evt.action.sender_id));
    publish_sse("music_change", std::move(j), area);
}
