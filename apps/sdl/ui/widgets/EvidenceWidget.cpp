#include "ui/widgets/EvidenceWidget.h"

#include "event/EventManager.h"
#include "event/EvidenceListEvent.h"

#include <imgui.h>

void EvidenceWidget::handle_events() {
    auto& ch = EventManager::instance().get_channel<EvidenceListEvent>();
    while (auto ev = ch.get_event()) {
        items_ = ev->items();
        selected_ = -1;
    }
}

void EvidenceWidget::render() {
    if (items_.empty()) {
        ImGui::TextDisabled("No evidence");
        return;
    }

    ImGui::BeginChild("##evi_list", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 3), ImGuiChildFlags_Borders);
    for (int i = 0; i < (int)items_.size(); i++) {
        ImGui::PushID(i);
        const char* label = items_[i].name.empty() ? "(unnamed)" : items_[i].name.c_str();
        if (ImGui::Selectable(label, selected_ == i))
            selected_ = i;
        ImGui::PopID();
    }
    ImGui::EndChild();

    if (selected_ >= 0 && selected_ < (int)items_.size()) {
        const auto& item = items_[selected_];
        if (!item.image.empty())
            ImGui::TextDisabled("Image: %s", item.image.c_str());
        ImGui::TextWrapped("%s", item.description.c_str());
    }
}
