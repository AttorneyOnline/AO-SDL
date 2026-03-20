#include "ao/event/OutgoingICMessageEvent.h"

#include <format>

OutgoingICMessageEvent::OutgoingICMessageEvent(ICMessageData data) : data_(std::move(data)) {
}

std::string OutgoingICMessageEvent::to_string() const {
    return std::format("OutgoingICMessageEvent(char={}, emote={}, msg={})", data_.character, data_.emote,
                       data_.message);
}
