#pragma once

#include "ui/IWidget.h"

class CharSelectScreen;
class RenderManager;

class CharSelectWidget : public IWidget {
  public:
    CharSelectWidget(CharSelectScreen& screen, RenderManager& render)
        : screen_(screen), render_(render) {}

    void handle_events() override;
    void render() override;

  private:
    CharSelectScreen& screen_;
    RenderManager& render_;
};
