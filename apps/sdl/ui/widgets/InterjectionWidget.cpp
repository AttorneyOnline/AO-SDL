#include "ui/widgets/InterjectionWidget.h"

#include "ui/widgets/ICMessageState.h"

#include <imgui.h>

void InterjectionWidget::handle_events() {
}

static bool toggle_button(const char* label, bool active) {
    if (active)
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    bool clicked = ImGui::Button(label);
    if (active)
        ImGui::PopStyleColor();
    return clicked;
}

void InterjectionWidget::render() {
    ImGui::Begin("Interjections");

    if (toggle_button("Hold It!", state_->objection_mod == 1))
        state_->objection_mod = (state_->objection_mod == 1) ? 0 : 1;

    ImGui::SameLine();
    if (toggle_button("Objection!", state_->objection_mod == 2))
        state_->objection_mod = (state_->objection_mod == 2) ? 0 : 2;

    ImGui::SameLine();
    if (toggle_button("Take That!", state_->objection_mod == 3))
        state_->objection_mod = (state_->objection_mod == 3) ? 0 : 3;

    ImGui::End();
}
