#pragma once

#include "Event.h"

#include <string>

class OutgoingChatEvent : public Event {
  public:
    OutgoingChatEvent(std::string sender_name, std::string message);

    std::string to_string() const override;
    std::string get_sender_name();
    std::string get_message();

  private:
    std::string sender_name;
    std::string message;
};
