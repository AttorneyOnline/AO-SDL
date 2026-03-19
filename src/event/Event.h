#pragma once

#include <string>

class Event {
  public:
    Event() = default;

    virtual std::string to_string() const;
};
