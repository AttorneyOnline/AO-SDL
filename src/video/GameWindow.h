#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include "render/RenderManager.h"
#include <SDL2/SDL.h>

class GameWindow {
  public:
    GameWindow();
    void start_loop(RenderManager& render);

  private:
    void init_sdl();

    SDL_Window* window;

    bool running;
};

#endif