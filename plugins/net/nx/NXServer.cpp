#include "NXServer.h"

#include "utils/Log.h"

#include <random>

/// Generate a random session token (hex string).
static std::string generate_token() {
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<uint32_t> dist;
    char buf[33];
    std::snprintf(buf, sizeof(buf), "%08x%08x%08x%08x", dist(rng), dist(rng), dist(rng), dist(rng));
    return buf;
}

NXServer::NXServer() = default;

std::string NXServer::create_session(uint64_t client_id) {
    ServerSession session;
    session.client_id = client_id;
    session.session_token = generate_token();
    session.protocol = "aonx";
    auto token = session.session_token;
    sessions_.emplace(client_id, std::move(session));
    Log::log_print(INFO, "NX: session created for client %llu", (unsigned long long)client_id);
    return token;
}

void NXServer::destroy_session(uint64_t client_id) {
    sessions_.erase(client_id);
    Log::log_print(INFO, "NX: session destroyed for client %llu", (unsigned long long)client_id);
}

ServerSession* NXServer::get_session(uint64_t client_id) {
    auto it = sessions_.find(client_id);
    return it != sessions_.end() ? &it->second : nullptr;
}

ServerSession* NXServer::get_session_by_token(const std::string& token) {
    for (auto& [id, session] : sessions_) {
        if (session.session_token == token)
            return &session;
    }
    return nullptr;
}

void NXServer::set_broadcast_func(BroadcastFunc func) {
    broadcast_func_ = std::move(func);
}

void NXServer::broadcast_to_area(const std::string& area, const NXMessage& msg) {
    if (!broadcast_func_)
        return;
    auto serialized = msg.serialize();
    for (auto& [id, session] : sessions_) {
        if (session.area == area)
            broadcast_func_(id, serialized);
    }
}

void NXServer::broadcast_all(const NXMessage& msg) {
    if (!broadcast_func_)
        return;
    auto serialized = msg.serialize();
    for (auto& [id, session] : sessions_)
        broadcast_func_(id, serialized);
}
