#pragma once

#include "ui/IWidget.h"

struct ICMessageState;

class InterjectionWidget : public IWidget {
  public:
    explicit InterjectionWidget(ICMessageState* state) : state_(state) {}

    void handle_events() override;
    void render() override;

  private:
    ICMessageState* state_;
};
