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
    ImGui::Begin("Chat");
    ImGui::Text("%s", m_buffer.c_str());
    ImGui::InputText("name", m_name, sizeof(m_name));
    ImGui::InputText("message", m_message, sizeof(m_message));
    if (ImGui::Button("Send")) {
        EventManager::instance().get_channel<OutgoingChatEvent>().publish(
            OutgoingChatEvent(std::string(m_name), std::string(m_message)));
        m_message[0] = '\0';
    }
    ImGui::End();
}
