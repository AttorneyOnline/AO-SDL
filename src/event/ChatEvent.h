#ifndef CHATEVENT_H
#define CHATEVENT_H

#include "Event.h"

class ChatEvent : public Event {
  public:
    ChatEvent(std::string sender_name, std::string message, bool system_message);

    std::string to_string() override;

  private:
    std::string sender_name;
    std::string message;
    bool system_message;
};

#endif