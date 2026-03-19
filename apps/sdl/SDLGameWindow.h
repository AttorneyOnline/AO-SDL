#pragma once

#include "render/RenderManager.h"
#include "ui/IUIRenderer.h"
#include "ui/UIManager.h"

#include <SDL2/SDL.h>

class SDLGameWindow {
  public:
    SDLGameWindow(UIManager& ui_manager);

    void start_loop(RenderManager& render, IUIRenderer& ui_renderer);

  private:
    void init_sdl();

    SDL_Window* window;
    UIManager& ui_manager;

    bool running;
};
