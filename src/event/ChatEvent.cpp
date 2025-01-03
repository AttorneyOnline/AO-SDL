#include "ChatEvent.h"

#include <format>

ChatEvent::ChatEvent(std::string sender_name, std::string message, bool system_message)
    : sender_name(sender_name), message(message), system_message(system_message) {
}

std::string ChatEvent::to_string() {
    return std::format("{}:{} {}", sender_name, message, system_message ? "(SYSTEM)" : "");
}