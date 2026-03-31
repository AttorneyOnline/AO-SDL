#include "NXServer.h"

#include "game/ClientId.h"
#include "utils/Crypto.h"
#include "utils/Log.h"

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

// -- Broadcast stubs (will push to SSE streams in Phase 5) -------------------

void NXServer::broadcast_ic(const std::string& area, const ICEvent& evt) {
    // TODO: push to SSE streams for NX clients in this area
    (void)area;
    (void)evt;
}

void NXServer::broadcast_ooc(const std::string& area, const OOCEvent& evt) {
    (void)area;
    (void)evt;
}

void NXServer::broadcast_char_select(const CharSelectEvent& evt) {
    (void)evt;
}

void NXServer::broadcast_chars_taken(const std::vector<int>& taken) {
    (void)taken;
}
