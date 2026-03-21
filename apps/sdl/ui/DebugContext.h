#pragma once

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
};
