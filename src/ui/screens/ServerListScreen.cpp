#include "ServerListScreen.h"

#include "ui/screens/CharSelectScreen.h"
#include "event/EventManager.h"
#include "event/ServerConnectEvent.h"
#include "event/ServerListEvent.h"

#include <imgui.h>

void ServerListScreen::enter(ScreenController& controller) {
    controller = &controller;
}

void ServerListScreen::exit() {
    controller = nullptr;
}

void ServerListScreen::handle_events() {
    auto& list_channel = EventManager::instance().get_channel<ServerListEvent>();
    while (auto optev = list_channel.get_event()) {
        servers = optev->get_server_list().get_servers();
    }

    if (pending_connect) {
        pending_connect = false;
        controller->push_screen(std::make_unique<CharSelectScreen>());
    }
}

void ServerListScreen::render(RenderManager& render) {
    render.begin_frame();

    ImGui::Begin("Servers");

    if (servers.empty()) {
        ImGui::Text("Fetching server list...");
    }
    else {
        if (ImGui::BeginTable("servers", 3,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Name",    ImGuiTableColumnFlags_WidthStretch, 2.0f);
            ImGui::TableSetupColumn("Players", ImGuiTableColumnFlags_WidthFixed,   60.0f);
            ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch, 3.0f);
            ImGui::TableHeadersRow();

            for (int i = 0; i < (int)servers.size(); i++) {
                const auto& s = servers[i];

                // Determine connectable WebSocket port. ws_port preferred over wss_port.
                std::optional<uint16_t> port = s.ws_port.has_value() ? s.ws_port : s.wss_port;

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                bool selected = (selected == i);
                ImGui::PushID(i);
                if (ImGui::Selectable(s.name.c_str(), selected,
                                      ImGuiSelectableFlags_SpanAllColumns,
                                      ImVec2(0, 0))) {
                    if (port.has_value()) {
                        selected = i;
                        pending_connect = true;
                        EventManager::instance().get_channel<ServerConnectEvent>().publish(
                            ServerConnectEvent(s.hostname, *port));
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

    ImGui::End();
    ImGui::Render();
}
