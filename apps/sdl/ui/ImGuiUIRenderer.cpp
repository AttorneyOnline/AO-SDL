#include "ImGuiUIRenderer.h"

#include "ao/ui/screens/CharSelectScreen.h"
#include "ao/ui/screens/CourtroomScreen.h"
#include "ao/ui/screens/ServerListScreen.h"
#include "ui/controllers/CharSelectController.h"
#include "ui/controllers/CourtroomController.h"
#include "ui/controllers/ServerListController.h"

#include <imgui.h>

void ImGuiUIRenderer::begin_frame() {
    ImGui::NewFrame();
}

IUIRenderer::NavAction ImGuiUIRenderer::pending_nav_action() {
    if (disconnect_modal_.should_return_to_server_list()) {
        disconnect_modal_.clear_flag();
        active_screen_id_.clear();
        return NavAction::POP_TO_ROOT;
    }
    if (controller_) {
        auto action = controller_->nav_action();
        if (action != NavAction::NONE) {
            active_screen_id_.clear();
            return action;
        }
    }
    return NavAction::NONE;
}

void ImGuiUIRenderer::render_screen(Screen& screen, RenderManager& render) {
    const auto& id = screen.screen_id();
    if (id != active_screen_id_) {
        controller_ = create_controller(screen, render);
        active_screen_id_ = id;
    }

    disconnect_modal_.handle_events();
    disconnect_modal_.render();

    if (controller_)
        controller_->render();
}

void ImGuiUIRenderer::end_frame() {
    ImGui::Render();
}

std::unique_ptr<IScreenController> ImGuiUIRenderer::create_controller(Screen& screen, RenderManager& render) {
    const auto& id = screen.screen_id();
    if (id == ServerListScreen::ID)
        return std::make_unique<ServerListController>(static_cast<ServerListScreen&>(screen), render);
    if (id == CharSelectScreen::ID)
        return std::make_unique<CharSelectController>(static_cast<CharSelectScreen&>(screen), render);
    if (id == CourtroomScreen::ID)
        return std::make_unique<CourtroomController>(static_cast<CourtroomScreen&>(screen), render);
    return nullptr;
}
