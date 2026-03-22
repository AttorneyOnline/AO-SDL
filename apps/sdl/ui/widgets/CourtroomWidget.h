#pragma once

#include "ui/IWidget.h"

class RenderManager;

class CourtroomWidget : public IWidget {
  public:
    explicit CourtroomWidget(RenderManager& render) : render_(render) {
    }

    void handle_events() override;
    void render() override;

  private:
    RenderManager& render_;
    int last_scale_ = 0;
};
