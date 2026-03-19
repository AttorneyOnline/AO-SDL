#include "ChatWidget.h"

#include "event/ChatEvent.h"
#include "event/EventManager.h"
#include "event/OutgoingChatEvent.h"

#include <format>
#include <imgui.h>

void ChatWidget::handle_events() {
    auto& channel = EventManager::instance().get_channel<ChatEvent>();
    while (auto optev = channel.get_event()) {
        buffer = std::format("{}\n{}", buffer, optev->to_string());
    }
}

void ChatWidget::render() {
    ImGui::Begin("Chat");
    ImGui::Text("%s", buffer.c_str());
    ImGui::InputText("name", name, IM_ARRAYSIZE(name));
    ImGui::InputText("message", message, IM_ARRAYSIZE(message));
    if (ImGui::Button("Send")) {
        EventManager::instance().get_channel<OutgoingChatEvent>().publish(
            OutgoingChatEvent(std::string(name), std::string(message)));
        message[0] = '\0';
    }
    ImGui::End();
}
