#pragma once

#include "ui/IWidget.h"

struct ICMessageState;

/// Displays defense and prosecution penalty bars (HP packet).
/// When the player is judge (side_index == 3), shows +/- controls.
class HealthBarWidget : public IWidget {
  public:
    explicit HealthBarWidget(ICMessageState* state) : state_(state) {}

    void handle_events() override;
    void render() override;

  private:
    ICMessageState* state_;
    int def_hp_ = 0; // 0-10
    int pro_hp_ = 0; // 0-10
};
