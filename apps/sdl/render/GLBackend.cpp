#include "IGPUBackend.h"

#include <SDL2/SDL.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

class GLBackend : public IGPUBackend {
  public:
    uint32_t window_flags() const override {
        return SDL_WINDOW_OPENGL;
    }

    void init(SDL_Window* window, IRenderer& /*renderer*/) override {
        window_ = window;

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        gl_context_ = SDL_GL_CreateContext(window);
        SDL_GL_SetSwapInterval(1);

        ImGui_ImplSDL2_InitForOpenGL(window, gl_context_);
        ImGui_ImplOpenGL3_Init();
    }

    void shutdown() override {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        if (gl_context_)
            SDL_GL_DeleteContext(gl_context_);
    }

    void begin_frame() override {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
    }

    void present() override {
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window_);
    }

  private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;
};

std::unique_ptr<IGPUBackend> create_gpu_backend() {
    return std::make_unique<GLBackend>();
}
