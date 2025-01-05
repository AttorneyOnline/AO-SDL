#include "UIEvent.h"

UIEvent::UIEvent(UIEventType type, EventTarget target) : Event(target), type(type) {
}

UIEventType UIEvent::get_type() {
    return type;
}

std::string UIEvent::to_string() const {
    switch (type) {
    case CHAR_LOADING_DONE:
        return "Character loading done";
    default:
        return "Generic UI Event (update UIEvent::to_string() lol)";
    }
}
