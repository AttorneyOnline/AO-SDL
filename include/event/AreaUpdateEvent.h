#pragma once

#include "event/Event.h"

#include <string>
#include <vector>

/// Published when the server sends an ARUP (area update) packet.
/// Each ARUP updates one metadata field for all areas at once.
class AreaUpdateEvent : public Event {
  public:
    enum Type { PLAYERS = 0, STATUS = 1, CM = 2, LOCK = 3 };

    AreaUpdateEvent(Type type, std::vector<std::string> values) : type_(type), values_(std::move(values)) {
    }

    std::string to_string() const override {
        return "ARUP type=" + std::to_string(type_) + " areas=" + std::to_string(values_.size());
    }

    Type type() const {
        return type_;
    }
    const std::vector<std::string>& values() const {
        return values_;
    }

  private:
    Type type_;
    std::vector<std::string> values_;
};
