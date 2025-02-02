#include "GameWindow.h"

#include "ui/UIManager.h"
#include "utils/Log.h"

#include <SDL2/SDL.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

GameWindow::GameWindow(UIManager& ui_manager) : window(nullptr), ui_manager(ui_manager), running(true) {
    init_sdl();
}

void GameWindow::start_loop(RenderManager& render) {
    SDL_Event event;

    while (running) {
        // Poll for input
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            ImGui_ImplSDL2_ProcessEvent(&event);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ui_manager.handle_events();
        ui_manager.render_current_view(render);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap
        SDL_GL_SwapWindow(window);

        // Log::log_print(INFO, "Rendered a frame");
        // todo: add some instrumentation to measure frame times, target a specific framerate to keep this thread from
        // gobbling CPU
    }
}

void GameWindow::init_sdl() {
    Log::log_print(LogLevel::DEBUG, "test %d %f", 1, 0.5f);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        Log::log_print(LogLevel::FATAL, "Failed to initialize SDL2: %s", SDL_GetError());
    }

    window = SDL_CreateWindow("SDL2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (!window) {
        Log::log_print(LogLevel::FATAL, "Failed to create window: %s", SDL_GetError());
    }

    // OpenGL 4.5
    // todo: find a better opengl level to target and make the shaders match. we don't need 4.5
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create context
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        Log::log_print(LogLevel::FATAL, "Failed to create OpenGL context: %s", SDL_GetError());
    }

    // Enable VSync
    if (SDL_GL_SetSwapInterval(1) < 0) {
        Log::log_print(LogLevel::ERR, "Failed to enable VSync: %s", SDL_GetError());
    }

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(0, 0);

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init();
}
