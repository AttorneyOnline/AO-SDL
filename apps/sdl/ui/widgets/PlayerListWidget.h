#pragma once

#include "ui/IWidget.h"

/// Displays the online player list (PR/PU packets) with area filtering.
class PlayerListWidget : public IWidget {
  public:
    void handle_events() override;
    void render() override;

  private:
    bool filter_by_area_ = true;
};
