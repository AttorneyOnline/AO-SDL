#include "ui/widgets/MessageOptionsWidget.h"

#include "ui/widgets/ICMessageState.h"

#include <imgui.h>

static constexpr const char* COLOR_LABELS[] = {"White", "Green", "Red", "Orange", "Blue", "Yellow", "Rainbow"};
static constexpr int COLOR_COUNT = 7;

void MessageOptionsWidget::handle_events() {
}

void MessageOptionsWidget::render() {
    ImGui::Checkbox("Flip", &state_->flip);
    ImGui::SameLine();
    ImGui::Checkbox("Realization", &state_->realization);
    ImGui::SameLine();
    ImGui::Checkbox("Screenshake", &state_->screenshake);
    ImGui::SameLine();
    ImGui::Checkbox("Additive", &state_->additive);

    ImGui::Combo("Text Color", &state_->text_color, COLOR_LABELS, COLOR_COUNT);
}
