#include "event/UIEvent.h"

UIEvent::UIEvent(UIEventType type) : type(type) {
}

UIEvent::UIEvent(UIEventType type, std::string character_name, int char_id)
    : type(type), character_name(std::move(character_name)), char_id(char_id) {
}

UIEventType UIEvent::get_type() {
    return type;
}

std::string UIEvent::to_string() const {
    switch (type) {
    case CHAR_LOADING_DONE:
        return "Character loading done";
    case ENTERED_COURTROOM:
        return "Entered courtroom as " + character_name;
    default:
        return "Generic UI Event";
    }
}
