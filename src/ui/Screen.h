#pragma once

#include "ScreenController.h"

#include "render/RenderManager.h"

class Screen {
  public:
    virtual ~Screen() = default;

    // Called when this screen becomes the top of the stack.
    // controller is valid for the lifetime of this screen's active period —
    // store it if you need to push/pop from handle_events().
    virtual void enter(ScreenController& controller) = 0;

    // Called when this screen is either popped or covered by a push.
    virtual void exit() = 0;

    // Process pending events for this frame.
    virtual void handle_events() = 0;

    // Render this screen for this frame.
    virtual void render(RenderManager& render) = 0;
};
