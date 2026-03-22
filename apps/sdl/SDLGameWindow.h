#pragma once

#include "render/IGPUBackend.h"
#include "render/RenderManager.h"
#include "ui/IUIRenderer.h"
#include "ui/UIManager.h"

#include <SDL2/SDL.h>

#include <functional>
#include <memory>

class SDLGameWindow {
  public:
    SDLGameWindow(UIManager& ui_manager, std::unique_ptr<IGPUBackend> backend);
    ~SDLGameWindow();

    /// Set a callback invoked once per frame before UI rendering.
    void set_frame_callback(std::function<void()> cb) {
        frame_callback_ = std::move(cb);
    }

    void start_loop(RenderManager& render, IUIRenderer& ui_renderer);

  private:
    SDL_Window* window;
    UIManager& ui_manager;
    std::unique_ptr<IGPUBackend> gpu;
    std::function<void()> frame_callback_;
    bool running;
};
