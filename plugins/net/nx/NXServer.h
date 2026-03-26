#pragma once

#include "NXMessage.h"
#include "game/GameAction.h"
#include "game/GameRoom.h"

#include <cstdint>
#include <functional>
#include <string>

class NXServer {
  public:
    explicit NXServer(GameRoom& room);

    using SendFunc = std::function<void(uint64_t client_id, const std::string& data)>;
    void set_send_func(SendFunc func);

    GameRoom& room() {
        return room_;
    }

    std::string create_session(uint64_t client_id);
    void destroy_session(uint64_t client_id);

    void send(uint64_t client_id, const NXMessage& msg);
    void send_to_area(const std::string& area, const NXMessage& msg);

    void broadcast_ic(const std::string& area, const ICEvent& evt);
    void broadcast_ooc(const std::string& area, const OOCEvent& evt);
    void broadcast_char_select(const CharSelectEvent& evt);
    void broadcast_chars_taken(const std::vector<int>& taken);

  private:
    GameRoom& room_;
    SendFunc send_func_;
};
