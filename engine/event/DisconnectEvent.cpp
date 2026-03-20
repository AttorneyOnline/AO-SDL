#include "event/DisconnectEvent.h"

DisconnectEvent::DisconnectEvent(std::string reason) : reason_(std::move(reason)) {
}

std::string DisconnectEvent::to_string() const {
    return "Disconnected: " + reason_;
}
