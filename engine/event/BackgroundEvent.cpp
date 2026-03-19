#include "event/BackgroundEvent.h"

#include <format>

BackgroundEvent::BackgroundEvent(std::string background, std::string position)
    : background(std::move(background)), position(std::move(position)) {}

std::string BackgroundEvent::to_string() const {
    return std::format("BackgroundEvent(bg={}, pos={})", background, position);
}
