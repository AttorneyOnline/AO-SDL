#include "ui/widgets/ServerListWidget.h"

#include "ao/ui/screens/ServerListScreen.h"
#include "render/RenderManager.h"

#include <imgui.h>

void ServerListWidget::handle_events() {
}

void ServerListWidget::render() {
    render_.begin_frame();

    const auto& servers = screen_.get_servers();

    if (servers.empty()) {
        ImGui::Text("Fetching server list...");
        return;
    }

    if (ImGui::BeginTable("servers", 3,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                              ImGuiTableFlags_SizingStretchProp,
                          ImGui::GetContentRegionAvail())) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Players", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableHeadersRow();

        for (int i = 0; i < (int)servers.size(); i++) {
            const auto& s = servers[i];
            std::optional<uint16_t> port = s.ws_port.has_value() ? s.ws_port : s.wss_port;

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            bool is_selected = (screen_.get_selected() == i);
            ImGui::PushID(i);
            if (ImGui::Selectable(s.name.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                if (port.has_value()) {
                    screen_.select_server(i);
                }
            }
            if (!port.has_value()) {
                ImGui::SetItemTooltip("TCP-only server (not supported)");
            }
            ImGui::PopID();

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", s.players);

            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(s.description.c_str());
        }

        ImGui::EndTable();
    }
}
