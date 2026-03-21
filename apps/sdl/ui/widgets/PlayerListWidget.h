#pragma once

#include "ui/IWidget.h"

#include <map>
#include <string>

/// Displays the online player list (PR/PU packets).
class PlayerListWidget : public IWidget {
  public:
    void handle_events() override;
    void render() override;

  private:
    struct PlayerInfo {
        std::string name;
        std::string character;
        std::string charname;
        int area_id = -1;
    };

    std::map<int, PlayerInfo> players_;
};
