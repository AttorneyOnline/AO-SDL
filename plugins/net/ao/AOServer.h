#pragma once

#include "AOPacket.h"
#include "game/GameAction.h"
#include "game/GameRoom.h"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class WebSocketServer;

struct AOProtocolState {
    std::string incomplete_buf;
    std::string hardware_id;

    enum State { CONNECTED, IDENTIFIED, LOADING, JOINED };
    State state = CONNECTED;
};

class AOServer {
  public:
    explicit AOServer(GameRoom& room);

    using SendFunc = std::function<void(uint64_t client_id, const std::string& data)>;
    void set_send_func(SendFunc func);

    void set_ws(WebSocketServer* ws) {
        ws_ = ws;
    }
    WebSocketServer* ws() const {
        return ws_;
    }

    GameRoom& room() {
        return room_;
    }

    void on_client_connected(uint64_t client_id);
    void on_client_disconnected(uint64_t client_id);
    void on_client_message(uint64_t client_id, const std::string& raw);

    AOProtocolState* get_protocol_state(uint64_t client_id);

    void send(uint64_t client_id, const AOPacket& packet);
    void send_to_area(const std::string& area, const AOPacket& packet);
    void send_to_all(const AOPacket& packet);

    void broadcast_ic(const std::string& area, const ICEvent& evt);
    void broadcast_ooc(const std::string& area, const OOCEvent& evt);
    void broadcast_music(const std::string& area, const MusicEvent& evt);
    void broadcast_char_select(const CharSelectEvent& evt);
    void broadcast_chars_taken(const std::vector<int>& taken);

    /// Send area-join state (BN, HP, LE) to a specific client.
    void send_area_join_info(uint64_t client_id, const std::string& area_name);

    /// Send the full player list (PR ADD + all PU fields) to a specific client.
    void send_player_list_snapshot(uint64_t client_id);

    /// Broadcast a PR/PU update to all joined AO2 clients.
    void broadcast_player_add(uint64_t session_id);
    void broadcast_player_remove(uint64_t session_id);
    void broadcast_player_update(uint64_t session_id, int data_type, const std::string& data);

  private:
    void dispatch(uint64_t client_id, AOPacket& packet);

    GameRoom& room_;
    SendFunc send_func_;
    WebSocketServer* ws_ = nullptr;
    std::unordered_map<uint64_t, AOProtocolState> proto_state_;
};
