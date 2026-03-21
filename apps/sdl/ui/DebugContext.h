#pragma once

#include <atomic>

class GameThread;
class IScenePresenter;

/// Minimal global context for debug overlay data sources.
/// Set once at startup, read by the debug widget.
struct DebugContext {
    static DebugContext& instance() {
        static DebugContext ctx;
        return ctx;
    }

    GameThread* game_thread = nullptr;
    IScenePresenter* presenter = nullptr;

    /// Internal resolution = BASE_W * internal_scale, BASE_H * internal_scale.
    /// Written by UI thread (debug widget), read by render loop.
    std::atomic<int> internal_scale{4};

    static constexpr int BASE_W = 256;
    static constexpr int BASE_H = 192;

    std::atomic<bool> wireframe{false};
};
