#include "ui/controllers/ServerListController.h"

ServerListController::ServerListController(ServerListScreen& screen, RenderManager& render)
    : server_list_(std::make_unique<ServerListWidget>(screen, render))
{
}

void ServerListController::render() {
    server_list_->render();
}
