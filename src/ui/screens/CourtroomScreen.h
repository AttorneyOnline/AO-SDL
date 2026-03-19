#pragma once

#include "ui/Screen.h"
#include "ui/ChatWidget.h"

class CourtroomScreen : public Screen {
  public:
    void enter(ScreenController& controller) override;
    void exit() override;
    void handle_events() override;
    void render(RenderManager& render) override;

  private:
    ChatWidget chat;
};
