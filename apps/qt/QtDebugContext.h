#pragma once

#include <atomic>

class GameThread;
class IScenePresenter;

/// Qt-frontend equivalent of the SDL DebugContext.
/// Holds raw pointers to engine objects needed by the debug overlay.
/// Written once at startup from main(); read by debug UI components.
struct QtDebugContext {
    static QtDebugContext& instance() {
        static QtDebugContext ctx;
        return ctx;
    }

    GameThread*       game_thread = nullptr;
    IScenePresenter*  presenter   = nullptr;

    /// Internal render resolution = BASE_W * internal_scale × BASE_H * internal_scale.
    /// Written by the debug overlay, read by the render loop on resize.
    std::atomic<int> internal_scale{4};

    static constexpr int BASE_W = 256;
    static constexpr int BASE_H = 192;
};
