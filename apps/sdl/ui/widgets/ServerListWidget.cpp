#include "ui/widgets/ServerListWidget.h"

#include "ao/ui/screens/ServerListScreen.h"
#include "render/RenderManager.h"

#include <imgui.h>

#include <cstdlib>
#include <cstring>
#include <string>

void ServerListWidget::handle_events() {
}

void ServerListWidget::render() {
    render_.begin_frame();

    const auto& servers = screen_.get_servers();

    if (servers.empty()) {
        ImGui::Text("Fetching server list...");
        return;
    }

    // Direct connect bar
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 80);
    bool enter_pressed = ImGui::InputTextWithHint("##direct", "host:port", direct_connect_buf_,
                                                  sizeof(direct_connect_buf_), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (ImGui::Button("Connect", ImVec2(72, 0)) || enter_pressed) {
        std::string addr(direct_connect_buf_);
        if (!addr.empty()) {
            uint16_t port = 27016; // AO2 default WS port
            auto colon = addr.rfind(':');
            if (colon != std::string::npos) {
                port = static_cast<uint16_t>(std::atoi(addr.c_str() + colon + 1));
                addr = addr.substr(0, colon);
            }
            screen_.direct_connect(addr, port);
        }
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
