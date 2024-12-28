#include "GameWindow.h"

#include "utils/Log.h"

#include <SDL2/SDL.h>

GameWindow::GameWindow() : window(nullptr), running(true) {
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
        }

        // Render our buffer
        render.render_frame();

        // Swap
        SDL_GL_SwapWindow(window);

        //Log::log_print(INFO, "Rendered a frame");
    }
}

void GameWindow::init_sdl() {
    Log::log_print(LogLevel::DEBUG, "test %d %f", 1, 0.5f);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        Log::log_print(LogLevel::FATAL, "Failed to initialize SDL2: %s", SDL_GetError());
    }

    window = SDL_CreateWindow("SDL2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    // Transform::set_aspect_ratio(640.0f / 480.0f);

    if (!window) {
        Log::log_print(LogLevel::FATAL, "Failed to create window: %s", SDL_GetError());
    }

    // OpenGL 3.1
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
        Log::log_print(LogLevel::ERROR, "Failed to enable VSync: %s", SDL_GetError());
    }
}