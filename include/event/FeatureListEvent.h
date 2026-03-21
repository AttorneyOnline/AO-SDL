#pragma once

#include "event/Event.h"

#include <string>
#include <vector>

/// Published when the server sends the feature list (FL packet).
class FeatureListEvent : public Event {
  public:
    FeatureListEvent(std::vector<std::string> features) : features_(std::move(features)) {}

    std::string to_string() const override { return std::to_string(features_.size()) + " features"; }

    const std::vector<std::string>& features() const { return features_; }

  private:
    std::vector<std::string> features_;
};
