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
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
    ImGui::InputTextMultiline("##ic_log", const_cast<char*>(buffer_.c_str()), buffer_.size() + 1,
                              ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
    ImGui::PopStyleColor(3);
}
