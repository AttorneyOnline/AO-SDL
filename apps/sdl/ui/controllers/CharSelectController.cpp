#include "ui/controllers/CharSelectController.h"

#include <imgui.h>

CharSelectController::CharSelectController(CharSelectScreen& screen, RenderManager& render)
    : char_select_(std::make_unique<CharSelectWidget>(screen, render)) {
}

void CharSelectController::render() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("##char_select_screen", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float left_width = avail.x * 0.65f;

    // Left column: character grid
    ImGui::BeginChild("##chars", ImVec2(left_width, 0), ImGuiChildFlags_Borders);
    ImGui::SeparatorText("Character Select");
    char_select_->render();
    ImGui::EndChild();

    ImGui::SameLine();

    // Right column: chat + disconnect
    ImGui::BeginChild("##chat_panel", ImVec2(0, 0), ImGuiChildFlags_Borders);
    ImGui::SeparatorText("Chat");
    chat_.handle_events();
    chat_.render();

    ImGui::Spacing();
    if (ImGui::Button("Disconnect", ImVec2(-1, 0)))
        nav_action_ = IUIRenderer::NavAction::POP_TO_ROOT;
    ImGui::EndChild();

    ImGui::End();
}

IUIRenderer::NavAction CharSelectController::nav_action() {
    auto action = nav_action_;
    nav_action_ = IUIRenderer::NavAction::NONE;
    return action;
}
