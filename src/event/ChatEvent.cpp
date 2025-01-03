#include "ChatEvent.h"

#include <format>

ChatEvent::ChatEvent(std::string sender_name, std::string message, bool system_message, EventTarget target)
    : sender_name(sender_name), message(message), system_message(system_message), Event(target) {
}

std::string ChatEvent::to_string() const {
    return std::format("{}: {} {}", sender_name, message, system_message ? "(SYSTEM)" : "");
}

std::string ChatEvent::get_sender_name() {
    return sender_name;
}

std::string ChatEvent::get_message() {
    return message;
}

bool ChatEvent::get_system_message() {
    return system_message;
}