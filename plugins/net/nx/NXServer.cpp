#include "NXServer.h"

#include "utils/Log.h"

#include <random>

static std::string generate_token() {
    thread_local std::mt19937 rng(std::random_device{}());
    thread_local std::uniform_int_distribution<uint32_t> dist;
    char buf[33];
    std::snprintf(buf, sizeof(buf), "%08x%08x%08x%08x", dist(rng), dist(rng), dist(rng), dist(rng));
    return buf;
}

NXServer::NXServer(GameRoom& room) : room_(room) {
    room_.add_ic_broadcast([this](const std::string& area, const ICEvent& evt) { broadcast_ic(area, evt); });
    room_.add_ooc_broadcast([this](const std::string& area, const OOCEvent& evt) { broadcast_ooc(area, evt); });
    room_.add_char_select_broadcast([this](const CharSelectEvent& evt) { broadcast_char_select(evt); });
    room_.add_chars_taken_broadcast([this](const std::vector<int>& taken) { broadcast_chars_taken(taken); });
}

void NXServer::set_send_func(SendFunc func) {
    send_func_ = std::move(func);
}

std::string NXServer::create_session(uint64_t client_id) {
    auto& session = room_.create_session(client_id, "aonx");
    session.session_token = generate_token();
    session.joined = true;
    Log::log_print(INFO, "NX: session created for client %llu", (unsigned long long)client_id);
    return session.session_token;
}

void NXServer::destroy_session(uint64_t client_id) {
    room_.destroy_session(client_id);
    Log::log_print(INFO, "NX: session destroyed for client %llu", (unsigned long long)client_id);
}

void NXServer::send(uint64_t client_id, const NXMessage& msg) {
    if (!send_func_)
        return;
    send_func_(client_id, msg.serialize());
}

void NXServer::send_to_area(const std::string& area, const NXMessage& msg) {
    if (!send_func_)
        return;
    auto serialized = msg.serialize();
    for (auto* session : room_.sessions_in_area(area)) {
        if (session->protocol == "aonx")
            send_func_(session->client_id, serialized);
    }
}

void NXServer::broadcast_ic(const std::string& area, const ICEvent& evt) {
    auto& a = evt.action;
    nlohmann::json j = {
        {"type", "ic_message"}, {"schema_version", 1}, {"character", a.character}, {"emote", a.emote},
        {"message", a.message}, {"side", a.side},      {"showname", a.showname},
    };
    send_to_area(area, NXMessage(j));
}

void NXServer::broadcast_ooc(const std::string& area, const OOCEvent& evt) {
    nlohmann::json j = {
        {"type", "ooc_message"},
        {"schema_version", 1},
        {"name", evt.action.name},
        {"message", evt.action.message},
    };
    send_to_area(area, NXMessage(j));
}

void NXServer::broadcast_char_select(const CharSelectEvent& evt) {
    nlohmann::json j = {
        {"type", "char_select"},
        {"schema_version", 1},
        {"client_id", evt.client_id},
        {"character_id", evt.character_id},
        {"character_name", evt.character_name},
    };
    send(evt.client_id, NXMessage(j));
}

void NXServer::broadcast_chars_taken(const std::vector<int>& taken) {
    nlohmann::json j = {
        {"type", "chars_taken"},
        {"schema_version", 1},
        {"taken", taken},
    };
    // TODO: broadcast to all NX clients when we have an all-NX-clients iterator
    (void)j;
}
