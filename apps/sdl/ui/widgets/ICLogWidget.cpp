#include "ui/widgets/ICLogWidget.h"

#include "event/ICLogEvent.h"
#include "event/EventManager.h"

#include <imgui.h>

#include <format>

void ICLogWidget::handle_events() {
    auto& channel = EventManager::instance().get_channel<ICLogEvent>();
    while (auto ev = channel.get_event()) {
        if (!buffer_.empty())
            buffer_ += '\n';
        buffer_ += std::format("[{}] {}", ev->get_showname(), ev->get_message());
    }
}

void ICLogWidget::render() {
    ImGui::BeginChild("##ic_log", ImVec2(0, 0), ImGuiChildFlags_None);
    ImGui::TextUnformatted(buffer_.c_str());
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f)
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
}
