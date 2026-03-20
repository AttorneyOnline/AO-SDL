#pragma once

#include "ui/controllers/IScreenController.h"
#include "ui/widgets/ServerListWidget.h"

#include <memory>

class ServerListScreen;
class RenderManager;

class ServerListController : public IScreenController {
  public:
    ServerListController(ServerListScreen& screen, RenderManager& render);
    void render() override;

  private:
    std::unique_ptr<ServerListWidget> server_list_;
};
