#pragma once

#include "render/IGPUBackend.h"

#include <imgui.h>
#include <imgui_impl_null.h>

/// Headless GPU backend for unit tests.
/// Derives from IGPUBackend and uses ImGui's null platform/renderer backends
/// so widget code can call ImGui functions without a real window or GPU.
class NullGPUBackend : public IGPUBackend {
  public:
    uint32_t window_flags() const override {
        return 0;
    }

    void create_context(SDL_Window* /*window*/) override {
        // No GPU context needed.
    }

    void init_imgui(SDL_Window* /*window*/, IRenderer& /*renderer*/) override {
        ImGui_ImplNull_Init();
    }

    void shutdown() override {
        ImGui_ImplNull_Shutdown();
    }

    void begin_frame() override {
        ImGui_ImplNull_NewFrame();
        ImGui::NewFrame();
    }

    void present() override {
        ImGui::Render();
        ImGui_ImplNullRender_RenderDrawData(ImGui::GetDrawData());
    }
};
