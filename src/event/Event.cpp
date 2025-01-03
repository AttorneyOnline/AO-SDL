#include "Event.h"

Event::Event(EventTarget target) : target(target) {
}

std::string Event::to_string() const {
    return "A generic Event";
}

EventTarget Event::get_target() const {
    return target;
}