#pragma once

#include "ui/IWidget.h"

struct ICMessageState;

class SideSelectWidget : public IWidget {
  public:
    explicit SideSelectWidget(ICMessageState* state) : state_(state) {}

    void handle_events() override;
    void render() override;

  private:
    ICMessageState* state_;
};
