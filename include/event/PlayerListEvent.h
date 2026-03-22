#pragma once

#include "event/Event.h"

#include <string>

/// Published when the server sends PR (player register) or PU (player update).
class PlayerListEvent : public Event {
  public:
    enum class Action { ADD, REMOVE, UPDATE_NAME, UPDATE_CHARACTER, UPDATE_CHARNAME, UPDATE_AREA };

    PlayerListEvent(Action action, int player_id, std::string data = "")
        : action_(action), player_id_(player_id), data_(std::move(data)) {
    }

    std::string to_string() const override {
        return "PlayerList action=" + std::to_string(static_cast<int>(action_)) + " id=" + std::to_string(player_id_);
    }

    Action action() const {
        return action_;
    }
    int player_id() const {
        return player_id_;
    }
    const std::string& data() const {
        return data_;
    }

  private:
    Action action_;
    int player_id_;
    std::string data_;
};
