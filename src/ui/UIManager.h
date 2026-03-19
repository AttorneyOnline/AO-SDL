#pragma once

#include "Screen.h"
#include "ScreenController.h"
#include "render/RenderManager.h"

#include <memory>
#include <vector>

class UIManager : public ScreenController {
  public:
    UIManager();

    // ScreenController interface
    void push_screen(std::unique_ptr<Screen> screen) override;
    void pop_screen() override;

    // Called each frame by the game loop.
    void handle_events();
    void render(RenderManager& render);

  private:
    Screen* top() const;

    std::vector<std::unique_ptr<Screen>> m_stack;
};
