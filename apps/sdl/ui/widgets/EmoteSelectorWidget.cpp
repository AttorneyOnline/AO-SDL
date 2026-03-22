#include "ui/widgets/EmoteSelectorWidget.h"

#include "ui/widgets/ICMessageState.h"

#include <imgui.h>

void EmoteSelectorWidget::handle_events() {
}

void EmoteSelectorWidget::render() {
    if (state_->emote_icons.empty()) {
        ImGui::Text("No emotes loaded");
        return;
    }

    float checkbox_height = ImGui::GetFrameHeightWithSpacing();
    float grid_height = ImGui::GetContentRegionAvail().y - checkbox_height;

    if (grid_height > 0) {
        ImGui::BeginChild("##emote_grid", ImVec2(0, grid_height), ImGuiChildFlags_None);

        const float icon_size = 40.0f;
        const float frame_padding = ImGui::GetStyle().FramePadding.x;
        const float item_spacing = ImGui::GetStyle().ItemSpacing.x;
        const float button_size = icon_size + frame_padding * 2.0f;
        const float panel_width = ImGui::GetContentRegionAvail().x;
        int columns = std::max(1, (int)((panel_width + item_spacing) / (button_size + item_spacing)));

        for (int i = 0; i < (int)state_->emote_icons.size(); i++) {
            const auto& entry = state_->emote_icons[i];

            ImGui::PushID(i);

            bool is_selected = (state_->selected_emote == i);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);

            bool clicked = false;
            if (entry.icon.has_value()) {
                ImTextureID tex = (ImTextureID)(uintptr_t)entry.icon->get_id();
                clicked = ImGui::ImageButton("emo", tex, ImVec2(icon_size, icon_size), {0, 1}, {1, 0});
            }
            else {
                clicked = ImGui::Button(entry.comment.c_str(), ImVec2(icon_size, icon_size));
            }

            if (is_selected)
                ImGui::PopStyleColor();

            if (clicked) {
                state_->selected_emote = i;
                // Auto-set Pre checkbox based on whether the emote has a pre-animation
                if (state_->char_sheet && i < state_->char_sheet->emote_count()) {
                    const auto& emo = state_->char_sheet->emote(i);
                    state_->pre_anim = !emo.pre_anim.empty() && emo.pre_anim != "-";
                }
            }

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", entry.comment.c_str());

            if ((i + 1) % columns != 0 && i + 1 < (int)state_->emote_icons.size())
                ImGui::SameLine();

            ImGui::PopID();
        }

        ImGui::EndChild();
    }

    ImGui::Checkbox("Pre-animation", &state_->pre_anim);
}
