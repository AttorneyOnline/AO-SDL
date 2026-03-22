#pragma once

#include "ui/IWidget.h"

struct ICMessageState;

class ICChatWidget : public IWidget {
  public:
    explicit ICChatWidget(ICMessageState* state) : state_(state) {
    }

    void handle_events() override;
    void render() override;

  private:
    void send();
    ICMessageState* state_;
};
