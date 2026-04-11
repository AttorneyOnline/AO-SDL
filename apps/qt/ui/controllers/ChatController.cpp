#include "ChatController.h"

#include "event/ChatEvent.h"
#include "event/EventManager.h"
#include "event/OutgoingChatEvent.h"

ChatController::ChatController(QObject* parent) : IQtScreenController(parent) {
}

void ChatController::drain() {
    auto& ch = EventManager::instance().get_channel<ChatEvent>();
    while (auto ev = ch.get_event()) {
        m_chat.appendLine({QString::fromStdString(ev->get_sender_name()), QString::fromStdString(ev->get_message()),
                           ev->get_system_message()});
    }
}

void ChatController::sendOOCMessage(const QString& name, const QString& message) {
    if (message.isEmpty())
        return;
    EventManager::instance().get_channel<OutgoingChatEvent>().publish(
        OutgoingChatEvent(name.toStdString(), message.toStdString()));
}

void ChatController::reset() {
    m_chat.clear();
}
