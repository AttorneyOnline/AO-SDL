#pragma once

#include "ui/IWidget.h"

struct ICMessageState;

class MessageOptionsWidget : public IWidget {
  public:
    explicit MessageOptionsWidget(ICMessageState* state) : state_(state) {
    }

    void handle_events() override;
    void render() override;

  private:
    ICMessageState* state_;
};
