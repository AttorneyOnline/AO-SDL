#include "ui/widgets/SideSelectWidget.h"

#include "ui/widgets/ICMessageState.h"

#include <imgui.h>

static constexpr const char* SIDE_LABELS[] = {"Defense", "Prosecution", "Witness", "Judge", "Jury", "Seance", "Helper"};
static constexpr int SIDE_COUNT = 7;

void SideSelectWidget::handle_events() {
}

void SideSelectWidget::render() {
    ImGui::Begin("Position");
    ImGui::Combo("Side", &state_->side_index, SIDE_LABELS, SIDE_COUNT);
    ImGui::End();
}
