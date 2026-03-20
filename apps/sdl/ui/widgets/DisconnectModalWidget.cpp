#include "ui/widgets/DisconnectModalWidget.h"

#include "event/DisconnectEvent.h"
#include "event/EventManager.h"

#include <imgui.h>

void DisconnectModalWidget::handle_events() {
    auto& channel = EventManager::instance().get_channel<DisconnectEvent>();
    while (auto ev = channel.get_event()) {
        reason_ = ev->get_reason();
        show_modal_ = true;
    }
}

void DisconnectModalWidget::render() {
    if (!show_modal_)
        return;

    ImGui::OpenPopup("Disconnected");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Disconnected", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Connection lost");
        if (!reason_.empty())
            ImGui::TextWrapped("%s", reason_.c_str());

        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            show_modal_ = false;
            return_to_server_list_ = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
