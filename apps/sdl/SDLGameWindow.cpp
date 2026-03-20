#include "SDLGameWindow.h"

#include "utils/Log.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>

SDLGameWindow::SDLGameWindow(UIManager& ui_manager, std::unique_ptr<IGPUBackend> backend)
    : window(nullptr), ui_manager(ui_manager), gpu(std::move(backend)), running(true) {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        Log::log_print(LogLevel::FATAL, "Failed to initialize SDL2: %s", SDL_GetError());
    }

    uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | gpu->window_flags();
    window = SDL_CreateWindow("SDL2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, flags);
    if (!window) {
        Log::log_print(LogLevel::FATAL, "Failed to create window: %s", SDL_GetError());
    }

    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowPadding = ImVec2(0, 0);
}

SDLGameWindow::~SDLGameWindow() {
    gpu->shutdown();
    ImGui::DestroyContext();
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
}

void SDLGameWindow::start_loop(RenderManager& render, IUIRenderer& ui_renderer) {
    gpu->init(window, render.get_renderer());

    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            ImGui_ImplSDL2_ProcessEvent(&event);
        }

        gpu->begin_frame();
        ui_manager.handle_events();

        Screen* screen = ui_manager.active_screen();
        if (screen) {
            ui_renderer.begin_frame();
            ui_renderer.render_screen(*screen, render);
            ui_renderer.end_frame();
        }

        auto nav = ui_renderer.pending_nav_action();
        if (nav == IUIRenderer::NavAction::POP_TO_ROOT)
            ui_manager.pop_to_root();
        else if (nav == IUIRenderer::NavAction::POP_SCREEN)
            ui_manager.pop_screen();

        gpu->present();
    }
}
