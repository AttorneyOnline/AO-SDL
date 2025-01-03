#include "EventManager.h"

#include <stdexcept>

EventManager::EventManager() : ui_event_channel(nullptr) {
}

void EventManager::set_ui_channel(std::unique_ptr<EventChannel<UIEvent>> ui_event_channel) {
    if (this->ui_event_channel.get() == nullptr) {
        this->ui_event_channel = std::move(ui_event_channel);
    }
    else {
        throw std::runtime_error("Tried to set ui_event_channel more than once");
    }
}

EventChannel<UIEvent>* EventManager::get_ui_channel() {
    return ui_event_channel.get();
}

void EventManager::set_chat_channel(std::unique_ptr<EventChannel<ChatEvent>> chat_event_channel) {
    if (this->chat_event_channel.get() == nullptr) {
        this->chat_event_channel = std::move(chat_event_channel);
    }
    else {
        throw std::runtime_error("Tried to set chat_event_channel more than once");
    }
}

EventChannel<ChatEvent>* EventManager::get_chat_channel() {
    return chat_event_channel.get();
}