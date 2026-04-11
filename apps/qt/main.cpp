// EngineInterface  — pure C++ engine services (no Qt dependency).
// QtAppInterface   — QObject layer wiring engine events into Qt via EngineEventBridge.
// main() handles render-pipeline setup (must precede QGuiApplication).

#include "EngineInterface.h"
#include "QtAppInterface.h"
#include "QtDebugContext.h"
#include "asset/MediaManager.h"
#include "event/EventManager.h"
#include "event/ServerListEvent.h"
#include "game/ServerList.h"
#include "net/HttpPool.h"
#include "render/IQtRenderBackend.h"
#include "render/RenderBridge.h"
#include "render/StateBuffer.h"
#include "utils/Log.h"

#include <QGuiApplication>
#include <QQuickWindow>

#include <cstdio>

int main(int argc, char* argv[]) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    Log::info("[main] starting");

#if !defined(Q_OS_APPLE)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    Log::debug("[main] graphics API set to OpenGL");
#endif

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName("Attorney Online");

    constexpr int render_w = 256 * 4;
    constexpr int render_h = 192 * 4;

    StateBuffer state_buffer;
    RenderBridge::instance().setStateBuffer(&state_buffer, render_w, render_h);
    Log::debug("[main] RenderBridge primed ({}x{})", render_w, render_h);

    auto gpu_backend = create_qt_render_backend();
    MediaManager::instance().assets().set_shader_backend(gpu_backend->backendName());
    QtDebugContext::instance().backend_name = gpu_backend->backendName();
    Log::debug("[main] shader backend set to '{}'", gpu_backend->backendName());

    EngineInterface engine(state_buffer);

    // Declared after engine — C++ destroys stack locals in reverse order,
    // so ~QtAppInterface() runs while QGuiApplication is still alive.
    QtAppInterface qt_iface;
    qt_iface.init(engine);

    if (!qt_iface.setup_qml()) {
        qt_iface.stop();
        engine.stop();
        MediaManager::instance().shutdown();
        return -1;
    }

    Log::info("[main] fetching server list");
    engine.http().get("http://servers.aceattorneyonline.com", "/servers", [](HttpResponse resp) {
        Log::debug("[main] server list HTTP response: {}", resp.status);
        if (resp.status == 200) {
            ServerList sv_list(resp.body);
            EventManager::instance().get_channel<ServerListEvent>().publish(ServerListEvent(std::move(sv_list)));
        }
    });

    qt_iface.start();
    Log::info("[main] entering event loop");

    int result = QGuiApplication::exec();

    Log::info("[main] event loop exited (code {}), shutting down", result);
    qt_iface.stop();
    engine.stop();
    MediaManager::instance().shutdown();
    Log::info("[main] shutdown complete");

    return result;
}
