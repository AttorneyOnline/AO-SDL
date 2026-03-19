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
    ScreenController* m_controller = nullptr;
    std::vector<ServerEntry> m_servers;
    int m_selected = -1;
    bool m_pending_connect = false;
};
