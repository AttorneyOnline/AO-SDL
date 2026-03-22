#include "ui/controllers/ServerListController.h"

#include <imgui.h>

ServerListController::ServerListController(ServerListScreen& screen, RenderManager& render)
    : server_list_(std::make_unique<ServerListWidget>(screen, render)) {
}

void ServerListController::render() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("##server_list_screen", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::SeparatorText("Servers");
    server_list_->render();

    ImGui::End();
}
