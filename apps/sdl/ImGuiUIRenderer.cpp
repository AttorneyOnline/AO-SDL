#include "ImGuiUIRenderer.h"

#include "ui/Screen.h"
#include "ui/ChatWidget.h"
#include "ui/screens/ServerListScreen.h"
#include "ui/screens/CharSelectScreen.h"
#include "ui/screens/CourtroomScreen.h"
#include "render/RenderManager.h"

#include <imgui.h>

void ImGuiUIRenderer::begin_frame() {
    ImGui::NewFrame();
}

void ImGuiUIRenderer::render_screen(Screen& screen, RenderManager& render) {
    const auto& id = screen.screen_id();

    if (id == ServerListScreen::ID) {
        render_server_list(static_cast<ServerListScreen&>(screen), render);
    }
    else if (id == CharSelectScreen::ID) {
        render_char_select(static_cast<CharSelectScreen&>(screen), render);
    }
    else if (id == CourtroomScreen::ID) {
        render_courtroom(static_cast<CourtroomScreen&>(screen), render);
    }
}

void ImGuiUIRenderer::end_frame() {
    ImGui::Render();
}

// --- Server List ---

void ImGuiUIRenderer::render_server_list(ServerListScreen& screen, RenderManager& render) {
    render.begin_frame();

    const auto& servers = screen.get_servers();

    ImGui::Begin("Servers");

    if (servers.empty()) {
        ImGui::Text("Fetching server list...");
    }
    else {
        if (ImGui::BeginTable("servers", 3,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Name",        ImGuiTableColumnFlags_WidthStretch, 2.0f);
            ImGui::TableSetupColumn("Players",     ImGuiTableColumnFlags_WidthFixed,   60.0f);
            ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch, 3.0f);
            ImGui::TableHeadersRow();

            for (int i = 0; i < (int)servers.size(); i++) {
                const auto& s = servers[i];
                std::optional<uint16_t> port = s.ws_port.has_value() ? s.ws_port : s.wss_port;

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                bool is_selected = (screen.get_selected() == i);
                ImGui::PushID(i);
                if (ImGui::Selectable(s.name.c_str(), is_selected,
                                      ImGuiSelectableFlags_SpanAllColumns)) {
                    if (port.has_value()) {
                        screen.select_server(i);
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
}

// --- Character Select ---

void ImGuiUIRenderer::render_char_select(CharSelectScreen& screen, RenderManager& render) {
    render.begin_frame();

    const auto& chars = screen.get_chars();

    ImGui::Begin("Character Select");

    if (chars.empty()) {
        ImGui::Text("Waiting for character list...");
    }
    else {
        const float icon_size = 64.0f;
        const float frame_padding = ImGui::GetStyle().FramePadding.x;
        const float item_spacing = ImGui::GetStyle().ItemSpacing.x;
        const float button_size = icon_size + frame_padding * 2.0f;
        const float panel_width = ImGui::GetContentRegionAvail().x;
        int columns = std::max(1, (int)((panel_width + item_spacing) / (button_size + item_spacing)));

        for (int i = 0; i < (int)chars.size(); i++) {
            const auto& entry = chars[i];

            ImGui::PushID(i);

            bool is_selected = (screen.get_selected() == i);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
            if (entry.taken)
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);

            bool clicked = false;
            if (entry.icon.has_value()) {
                ImTextureID tex = (ImTextureID)(uintptr_t)entry.icon->get_id();
                clicked = ImGui::ImageButton("icon", tex, ImVec2(icon_size, icon_size), {0, 1}, {1, 0});
            }
            else {
                clicked = ImGui::Button(entry.folder.c_str(), ImVec2(icon_size, icon_size));
            }

            if (is_selected)  ImGui::PopStyleColor();
            if (entry.taken)  ImGui::PopStyleColor();

            if (clicked) {
                screen.select_character(i);
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s%s", entry.folder.c_str(), entry.taken ? " (taken)" : "");
            }

            if ((i + 1) % columns != 0 && i + 1 < (int)chars.size()) {
                ImGui::SameLine();
            }

            ImGui::PopID();
        }
    }

    ImGui::End();

    render_chat(screen.get_chat());
}

// --- Courtroom ---

void ImGuiUIRenderer::render_courtroom(CourtroomScreen& screen, RenderManager& render) {
    uint32_t render_texture = render.render_frame();

    render.begin_frame();

    ImGui::Begin("Courtroom");
    ImGui::Image(render_texture, ImGui::GetContentRegionAvail(), {0, 1}, {1, 0});
    ImGui::End();

    render_chat(screen.get_chat());
}

// --- Chat (shared widget) ---

void ImGuiUIRenderer::render_chat(ChatWidget& chat) {
    ImGui::Begin("Chat");
    ImGui::Text("%s", chat.get_buffer().c_str());
    ImGui::InputText("name", chat.name_buf(), chat.name_buf_size());
    ImGui::InputText("message", chat.message_buf(), chat.message_buf_size());
    if (ImGui::Button("Send")) {
        chat.send_message();
    }
    ImGui::End();
}
