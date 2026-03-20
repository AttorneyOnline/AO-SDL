#include "ImGuiUIRenderer.h"

#include "ao/ui/screens/CharSelectScreen.h"
#include "ao/ui/screens/CourtroomScreen.h"
#include "ao/ui/screens/ServerListScreen.h"
#include "ui/Screen.h"

#include <imgui.h>

void ImGuiUIRenderer::begin_frame() {
    ImGui::NewFrame();
}

void ImGuiUIRenderer::render_screen(Screen& screen, RenderManager& render) {
    bind_screen(screen, render);

    const auto& id = screen.screen_id();

    if (id == ServerListScreen::ID) {
        server_list_->render();
    }
    else if (id == CharSelectScreen::ID) {
        char_select_->render();
        chat_.handle_events();
        chat_.render();
    }
    else if (id == CourtroomScreen::ID) {
        courtroom_->render();
        chat_.handle_events();
        chat_.render();
    }
}

void ImGuiUIRenderer::end_frame() {
    ImGui::Render();
}

void ImGuiUIRenderer::bind_screen(Screen& screen, RenderManager& render) {
    const auto& id = screen.screen_id();
    if (id == active_screen_id_)
        return;

    active_screen_id_ = id;

    if (id == ServerListScreen::ID) {
        server_list_ = std::make_unique<ServerListWidget>(static_cast<ServerListScreen&>(screen), render);
    }
    else if (id == CharSelectScreen::ID) {
        char_select_ = std::make_unique<CharSelectWidget>(static_cast<CharSelectScreen&>(screen), render);
    }
    else if (id == CourtroomScreen::ID) {
        courtroom_ = std::make_unique<CourtroomWidget>(render);
    }
}
