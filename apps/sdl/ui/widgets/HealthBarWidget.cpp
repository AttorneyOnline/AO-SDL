#include "ui/widgets/HealthBarWidget.h"

#include "event/EventManager.h"
#include "event/HealthBarEvent.h"
#include "event/OutgoingHealthBarEvent.h"
#include "ui/widgets/ICMessageState.h"

#include <imgui.h>

#include <algorithm>

void HealthBarWidget::handle_events() {
    auto& ch = EventManager::instance().get_channel<HealthBarEvent>();
    while (auto ev = ch.get_event()) {
        int val = std::clamp(ev->value(), 0, 10);
        if (ev->side() == 1)
            def_hp_ = val;
        else if (ev->side() == 2)
            pro_hp_ = val;
    }
}

void HealthBarWidget::render() {
    float w = ImGui::GetContentRegionAvail().x;
    float bar_h = 8.0f;
    bool is_judge = state_ && state_->side_index == 3;

    auto draw_bar = [&](const char* label, int side, int& hp, ImU32 color) {
        if (is_judge) {
            ImGui::PushID(side);
            if (ImGui::SmallButton("-") && hp > 0) {
                hp--;
                EventManager::instance().get_channel<OutgoingHealthBarEvent>().publish(
                    OutgoingHealthBarEvent(side, hp));
            }
            ImGui::SameLine();
        }

        ImGui::Text("%s", label);
        ImGui::SameLine(is_judge ? 70.0f : 35.0f);
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float fill = (float)hp / 10.0f;
        float bar_w = w - (is_judge ? 110.0f : 40.0f);
        dl->AddRectFilled(p, {p.x + bar_w, p.y + bar_h}, IM_COL32(40, 40, 40, 255), 2.0f);
        if (hp > 0)
            dl->AddRectFilled(p, {p.x + bar_w * fill, p.y + bar_h}, color, 2.0f);
        ImGui::Dummy(ImVec2(bar_w, bar_h + 2.0f));

        if (is_judge) {
            ImGui::SameLine();
            if (ImGui::SmallButton("+") && hp < 10) {
                hp++;
                EventManager::instance().get_channel<OutgoingHealthBarEvent>().publish(
                    OutgoingHealthBarEvent(side, hp));
            }
            ImGui::PopID();
        }
    };

    draw_bar("DEF", 1, def_hp_, IM_COL32(80, 180, 80, 255));
    draw_bar("PRO", 2, pro_hp_, IM_COL32(200, 60, 60, 255));
}
