#pragma once

#include "Event.h"

#include <string>

class DisconnectEvent : public Event {
  public:
    explicit DisconnectEvent(std::string reason);

    std::string to_string() const override;

    const std::string& get_reason() const {
        return reason_;
    }

  private:
    std::string reason_;
};
