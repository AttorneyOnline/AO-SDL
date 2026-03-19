#pragma once

#include <memory>

class Screen;

// Minimal interface exposed to screens so they can drive stack transitions
// without depending on UIManager directly.
class ScreenController {
  public:
    virtual void push_screen(std::unique_ptr<Screen> screen) = 0;
    virtual void pop_screen() = 0;

    virtual ~ScreenController() = default;
};
