#include "ui/widgets/CharSelectWidget.h"

#include "ao/ui/screens/CharSelectScreen.h"
#include "render/RenderManager.h"

#include <imgui.h>

void CharSelectWidget::handle_events() {
}

void CharSelectWidget::render() {
    render_.begin_frame();

    const auto& chars = screen_.get_chars();

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

            bool is_selected = (screen_.get_selected() == i);
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

            if (is_selected)
                ImGui::PopStyleColor();
            if (entry.taken)
                ImGui::PopStyleColor();

            if (clicked) {
                screen_.select_character(i);
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
}
