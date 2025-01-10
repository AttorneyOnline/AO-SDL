#pragma once

#include "Event.h"

enum UIEventType { CHAR_LOADING_DONE };

class UIEvent : public Event {
  public:
    UIEvent(UIEventType type, EventTarget target);

    std::string to_string() const override;
    UIEventType get_type();

  private:
    UIEventType type;
};
