#include "ui/ChatWidget.h"

#include "event/ChatEvent.h"
#include "event/EventManager.h"
#include "event/OutgoingChatEvent.h"

#include <format>

void ChatWidget::handle_events() {
    auto& channel = EventManager::instance().get_channel<ChatEvent>();
    while (auto optev = channel.get_event()) {
        m_buffer = std::format("{}\n{}", m_buffer, optev->to_string());
    }
}

void ChatWidget::send_message() {
    EventManager::instance().get_channel<OutgoingChatEvent>().publish(
        OutgoingChatEvent(std::string(m_name), std::string(m_message)));
    m_message[0] = '\0';
}
