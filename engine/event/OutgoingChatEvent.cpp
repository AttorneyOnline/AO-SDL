#include "event/OutgoingChatEvent.h"

#include <format>

OutgoingChatEvent::OutgoingChatEvent(std::string sender_name, std::string message)
    : sender_name(sender_name), message(message) {
}

std::string OutgoingChatEvent::to_string() const {
    return std::format("{}: {}", sender_name, message);
}

std::string OutgoingChatEvent::get_sender_name() {
    return sender_name;
}

std::string OutgoingChatEvent::get_message() {
    return message;
}
