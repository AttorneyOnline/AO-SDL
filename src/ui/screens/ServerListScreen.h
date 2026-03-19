#pragma once

#include "ui/Screen.h"
#include "game/ServerList.h"

#include <vector>

class ServerListScreen : public Screen {
  public:
    void enter(ScreenController& controller) override;
    void exit() override;
    void handle_events() override;
    void render(RenderManager& render) override;

  private:
    ScreenController* controller = nullptr;
    std::vector<ServerEntry> servers;
    int selected = -1;
    bool pending_connect = false;
};
