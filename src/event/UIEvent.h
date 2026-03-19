#pragma once

#include "Event.h"

enum UIEventType { CHAR_LOADING_DONE, ENTERED_COURTROOM };

class UIEvent : public Event {
  public:
    UIEvent(UIEventType type);

    std::string to_string() const override;
    UIEventType get_type();

  private:
    UIEventType type;
};
