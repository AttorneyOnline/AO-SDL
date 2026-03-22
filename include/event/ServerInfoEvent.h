#pragma once

#include "event/Event.h"

#include <string>

/// Published when the server identifies itself (ID packet).
class ServerInfoEvent : public Event {
  public:
    ServerInfoEvent(std::string software, std::string version, int player_num)
        : software(std::move(software)), version(std::move(version)), player_num(player_num) {
    }

    std::string to_string() const override {
        return software + " " + version;
    }

    const std::string& get_software() const {
        return software;
    }
    const std::string& get_version() const {
        return version;
    }
    int get_player_num() const {
        return player_num;
    }

  private:
    std::string software;
    std::string version;
    int player_num;
};
