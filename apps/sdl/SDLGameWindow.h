#pragma once

#include "IGPUBackend.h"
#include "render/RenderManager.h"
#include "ui/IUIRenderer.h"
#include "ui/UIManager.h"

#include <SDL2/SDL.h>

#include <memory>

class SDLGameWindow {
  public:
    SDLGameWindow(UIManager& ui_manager, std::unique_ptr<IGPUBackend> backend);
    ~SDLGameWindow();

    void start_loop(RenderManager& render, IUIRenderer& ui_renderer);

  private:
    SDL_Window* window;
    UIManager& ui_manager;
    std::unique_ptr<IGPUBackend> gpu;
    bool running;
};
