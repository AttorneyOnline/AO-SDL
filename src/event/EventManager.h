#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include "ChatEvent.h"
#include "Event.h"
#include "EventChannel.h"
#include "UIEvent.h"

#include <memory>

class EventManager {
  public:
    EventManager();

    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;

    static EventManager& get_instance() {
        static EventManager instance; // Guaranteed to be destroyed and instantiated on first use
        return instance;
    }

    void set_ui_channel(std::unique_ptr<EventChannel<UIEvent>> ui_event_channel);
    EventChannel<UIEvent>* get_ui_channel();

    void set_chat_channel(std::unique_ptr<EventChannel<ChatEvent>> chat_event_channel);
    EventChannel<ChatEvent>* get_chat_channel();

  private:
    std::unique_ptr<EventChannel<UIEvent>> ui_event_channel;
    std::unique_ptr<EventChannel<ChatEvent>> chat_event_channel;
};

#endif