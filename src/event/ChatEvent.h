#ifndef CHATEVENT_H
#define CHATEVENT_H

#include "Event.h"

class ChatEvent : public Event {
  public:
    ChatEvent(std::string sender_name, std::string message, bool system_message, EventTarget target);

    std::string to_string() const override;
    std::string get_sender_name();
    std::string get_message();
    bool get_system_message();

  private:
    std::string sender_name;
    std::string message;
    bool system_message;
};

#endif