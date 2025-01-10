#pragma once

#include "render/RenderManager.h"
#include "ui/UIManager.h"

#include <SDL2/SDL.h>

class GameWindow {
  public:
    GameWindow(UIManager& ui_manager);

    void start_loop(RenderManager& render);

  private:
    void init_sdl();

    SDL_Window* window;
    UIManager& ui_manager;

    bool running;
};
