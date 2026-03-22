#pragma once

#include "ui/IWidget.h"

struct ICMessageState;

class EmoteSelectorWidget : public IWidget {
  public:
    explicit EmoteSelectorWidget(ICMessageState* state) : state_(state) {
    }

    void handle_events() override;
    void render() override;

  private:
    ICMessageState* state_;
};
