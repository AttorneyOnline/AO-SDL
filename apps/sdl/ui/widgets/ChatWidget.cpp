#include "ui/widgets/ChatWidget.h"

#include "event/ChatEvent.h"
#include "event/EventManager.h"
#include "event/OutgoingChatEvent.h"

#include <imgui.h>

std::string& ChatWidget::shared_buffer() {
    static std::string buf;
    return buf;
}

void ChatWidget::handle_events() {
    auto& channel = EventManager::instance().get_channel<ChatEvent>();
    auto& buf = shared_buffer();
    while (auto optev = channel.get_event()) {
        if (!buf.empty())
            buf += '\n';
        buf += optev->to_string();
    }
}

void ChatWidget::render() {
    auto& buf = shared_buffer();
    float input_height = ImGui::GetFrameHeightWithSpacing() * 3 + ImGui::GetStyle().ItemSpacing.y;
    float log_height = ImGui::GetContentRegionAvail().y - input_height;

    if (log_height > 0) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
        ImGui::InputTextMultiline("##chat_log", const_cast<char*>(buf.c_str()), buf.size() + 1, ImVec2(-1, log_height),
                                  ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
        ImGui::PopStyleColor(3);
    }

    ImGui::InputText("Name", m_name, sizeof(m_name));
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Send").x -
                            ImGui::GetStyle().FramePadding.x * 2 - ImGui::GetStyle().ItemSpacing.x);

    auto send = [this]() {
        std::string msg(m_message);
        m_message[0] = '\0';
        if (msg == "/debug") {
            debug_toggled_ = true;
            return;
        }
        EventManager::instance().get_channel<OutgoingChatEvent>().publish(
            OutgoingChatEvent(std::string(m_name), std::move(msg)));
    };

    if (ImGui::InputText("##ooc_msg", m_message, sizeof(m_message), ImGuiInputTextFlags_EnterReturnsTrue))
        send();
    ImGui::SameLine();
    if (ImGui::Button("Send"))
        send();
}
