#pragma once

#include "ui/IUIRenderer.h"

class IScreenController {
  public:
    virtual ~IScreenController() = default;
    virtual void render() = 0;
    virtual IUIRenderer::NavAction nav_action() { return IUIRenderer::NavAction::NONE; }
};
