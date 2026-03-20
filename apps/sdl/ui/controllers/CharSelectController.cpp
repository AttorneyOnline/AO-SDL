#include "ui/controllers/CharSelectController.h"

#include <imgui.h>

CharSelectController::CharSelectController(CharSelectScreen& screen, RenderManager& render)
    : char_select_(std::make_unique<CharSelectWidget>(screen, render))
{
}

void CharSelectController::render() {
    char_select_->render();
    chat_.handle_events();
    chat_.render();

    ImGui::Begin("Connection");
    if (ImGui::Button("Disconnect"))
        nav_action_ = IUIRenderer::NavAction::POP_TO_ROOT;
    ImGui::End();
}

IUIRenderer::NavAction CharSelectController::nav_action() {
    auto action = nav_action_;
    nav_action_ = IUIRenderer::NavAction::NONE;
    return action;
}
