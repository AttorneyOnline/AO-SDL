#include "AOServer.h"

#include "PacketTypes.h"
#include "utils/Log.h"

AOServer::AOServer() {
    ao_register_packet_types();
}

void AOServer::set_send_func(SendFunc func) {
    send_func_ = std::move(func);
}

void AOServer::on_client_connected(uint64_t client_id) {
    ClientEntry entry;
    entry.session.client_id = client_id;
    entry.session.session_id = next_player_number_;
    entry.session.protocol = "ao2";
    sessions_.emplace(client_id, std::move(entry));

    Log::log_print(INFO, "AO: client %llu connected (player %d)", (unsigned long long)client_id, next_player_number_);
    next_player_number_++;

    // Initiate handshake: send decryptor (NOENCRYPT disables FantaCrypt)
    send(client_id, AOPacket("decryptor", {"NOENCRYPT"}));
}

void AOServer::on_client_disconnected(uint64_t client_id) {
    auto it = sessions_.find(client_id);
    if (it != sessions_.end()) {
        // Free character slot if taken
        int char_id = it->second.session.character_id;
        if (char_id >= 0 && char_id < static_cast<int>(game_state_.char_taken.size()))
            game_state_.char_taken[char_id] = 0;
    }
    sessions_.erase(client_id);
    Log::log_print(INFO, "AO: client %llu disconnected", (unsigned long long)client_id);
}

void AOServer::on_client_message(uint64_t client_id, const std::string& raw) {
    auto it = sessions_.find(client_id);
    if (it == sessions_.end())
        return;

    auto& entry = it->second;
    entry.proto.incomplete_buf += raw;

    std::string& buf = entry.proto.incomplete_buf;
    const std::string delim = AOPacket::DELIMITER;

    size_t pos;
    while ((pos = buf.find(delim)) != std::string::npos) {
        std::string packet_str = buf.substr(0, pos + delim.size());
        buf.erase(0, pos + delim.size());

        Log::log_print(VERBOSE, "AO [%llu] <<< %s", (unsigned long long)client_id, packet_str.c_str());

        auto packet = AOPacket::deserialize(packet_str);
        if (packet) {
            try {
                dispatch(entry, *packet);
            }
            catch (const std::exception& e) {
                Log::log_print(WARNING, "AO: error handling packet from client %llu: %s", (unsigned long long)client_id,
                               e.what());
            }
        }
    }
}

ServerSession* AOServer::get_session(uint64_t client_id) {
    auto it = sessions_.find(client_id);
    return it != sessions_.end() ? &it->second.session : nullptr;
}

AOProtocolState* AOServer::get_protocol_state(uint64_t client_id) {
    auto it = sessions_.find(client_id);
    return it != sessions_.end() ? &it->second.proto : nullptr;
}

void AOServer::send(uint64_t client_id, const AOPacket& packet) {
    if (!send_func_)
        return;
    auto serialized = packet.serialize();
    Log::log_print(VERBOSE, "AO [%llu] >>> %s", (unsigned long long)client_id, serialized.c_str());
    send_func_(client_id, serialized);
}

void AOServer::broadcast_to_area(const std::string& area, const AOPacket& packet) {
    if (!send_func_)
        return;
    auto serialized = packet.serialize();
    Log::log_print(VERBOSE, "AO [area:%s] >>> %s", area.c_str(), serialized.c_str());
    for (auto& [id, entry] : sessions_) {
        if (entry.session.area == area)
            send_func_(id, serialized);
    }
}

void AOServer::broadcast_all(const AOPacket& packet) {
    if (!send_func_)
        return;
    auto serialized = packet.serialize();
    Log::log_print(VERBOSE, "AO [all] >>> %s", serialized.c_str());
    for (auto& [id, entry] : sessions_)
        send_func_(id, serialized);
}

void AOServer::dispatch(ClientEntry& entry, AOPacket& packet) {
    packet.handle_server(*this, entry.session);
}
