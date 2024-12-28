#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <SDL2/SDL.h>
#include <RenderManager.h>

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