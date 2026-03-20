#pragma once

#include "Event.h"

#include <string>

enum UIEventType {
    CHAR_LOADING_DONE,
    ENTERED_COURTROOM
};

class UIEvent : public Event {
  public:
    UIEvent(UIEventType type);
    UIEvent(UIEventType type, std::string character_name, int char_id);

    std::string to_string() const override;

    UIEventType get_type();

    const std::string& get_character_name() const {
        return character_name;
    }
    int get_char_id() const {
        return char_id;
    }

  private:
    UIEventType type;
    std::string character_name;
    int char_id = -1;
};
