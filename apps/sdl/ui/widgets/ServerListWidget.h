#pragma once

#include "ui/IWidget.h"

class ServerListScreen;
class RenderManager;

class ServerListWidget : public IWidget {
  public:
    ServerListWidget(ServerListScreen& screen, RenderManager& render) : screen_(screen), render_(render) {
    }

    void handle_events() override;
    void render() override;

  private:
    ServerListScreen& screen_;
    RenderManager& render_;
};
