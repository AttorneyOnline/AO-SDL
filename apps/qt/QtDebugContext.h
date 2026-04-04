#pragma once

#include <atomic>

class GameThread;
class IScenePresenter;

/// Qt-frontend equivalent of the SDL DebugContext.
///
/// Holds raw pointers to engine objects needed by the debug overlay.
/// Written once at startup (via EngineInterface); read by debug UI
/// components.  The SDL variant also holds an SDLAudioDevice pointer —
/// that will be added here once AudioThread is wired into the Qt frontend.
struct QtDebugContext {
    static QtDebugContext& instance() {
        static QtDebugContext ctx;
        return ctx;
    }

    GameThread* game_thread = nullptr;
    IScenePresenter* presenter = nullptr;

    /// Backend name ("OpenGL" or "Metal").  Set once by main() after
    /// create_qt_render_backend(); read by DebugController.
    const char* backend_name = "unknown";

    /// Internal render resolution = BASE_W * internal_scale x BASE_H * internal_scale.
    /// Written by the debug overlay, read by the render loop on resize.
    std::atomic<int> internal_scale{4};

    /// Wireframe toggle for the renderer.  Written by the debug overlay,
    /// read by the courtroom widget on each frame.
    std::atomic<bool> wireframe{false};

    static constexpr int BASE_W = 256;
    static constexpr int BASE_H = 192;
};
