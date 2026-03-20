#include "ui/widgets/ChatWidget.h"

#include "event/ChatEvent.h"
#include "event/EventManager.h"
#include "event/OutgoingChatEvent.h"

#include <imgui.h>

#include <format>

void ChatWidget::handle_events() {
    auto& channel = EventManager::instance().get_channel<ChatEvent>();
    while (auto optev = channel.get_event()) {
        m_buffer = std::format("{}\n{}", m_buffer, optev->to_string());
    }
}

void ChatWidget::render() {
    float input_height = ImGui::GetFrameHeightWithSpacing() * 3 + ImGui::GetStyle().ItemSpacing.y;
    float log_height = ImGui::GetContentRegionAvail().y - input_height;

    if (log_height > 0) {
        ImGui::BeginChild("##chat_log", ImVec2(0, log_height), ImGuiChildFlags_None);
        ImGui::TextUnformatted(m_buffer.c_str());
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f)
            ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();
    }

    ImGui::InputText("Name", m_name, sizeof(m_name));
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Send").x -
                            ImGui::GetStyle().FramePadding.x * 2 - ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::InputText("##ooc_msg", m_message, sizeof(m_message), ImGuiInputTextFlags_EnterReturnsTrue)) {
        EventManager::instance().get_channel<OutgoingChatEvent>().publish(
            OutgoingChatEvent(std::string(m_name), std::string(m_message)));
        m_message[0] = '\0';
    }
    ImGui::SameLine();
    if (ImGui::Button("Send")) {
        EventManager::instance().get_channel<OutgoingChatEvent>().publish(
            OutgoingChatEvent(std::string(m_name), std::string(m_message)));
        m_message[0] = '\0';
    }
}
