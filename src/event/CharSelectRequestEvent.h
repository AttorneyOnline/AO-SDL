#pragma once

#include "Event.h"

class CharSelectRequestEvent : public Event {
  public:
    explicit CharSelectRequestEvent(int char_id);
    int get_char_id() const;

  private:
    int m_char_id;
};
