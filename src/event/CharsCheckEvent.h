#pragma once

#include "Event.h"

#include <vector>

class CharsCheckEvent : public Event {
  public:
    explicit CharsCheckEvent(std::vector<bool> taken);
    const std::vector<bool>& get_taken() const;

  private:
    std::vector<bool> m_taken;
};
