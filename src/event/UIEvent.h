#ifndef UIEVENT_H
#define UIEVENT_H

#include "Event.h"

enum UIEventType { CHAR_LOADING_DONE };

class UIEvent : public Event {
  public:
    UIEvent(UIEventType type);

    std::string to_string() override;
    UIEventType get_type();

  private:
    UIEventType type;
};

#endif