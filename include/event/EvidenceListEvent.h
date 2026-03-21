#pragma once

#include "event/Event.h"

#include <string>
#include <vector>

struct EvidenceItem {
    std::string name;
    std::string description;
    std::string image;
};

/// Published when the server sends LE (evidence list).
class EvidenceListEvent : public Event {
  public:
    EvidenceListEvent(std::vector<EvidenceItem> items) : items_(std::move(items)) {}

    std::string to_string() const override { return std::to_string(items_.size()) + " evidence items"; }

    const std::vector<EvidenceItem>& items() const { return items_; }

  private:
    std::vector<EvidenceItem> items_;
};
