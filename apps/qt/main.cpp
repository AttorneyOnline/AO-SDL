// The SDL frontend keeps all wiring in main() because SDL's event loop is an
// explicit poll — the application owns the loop and calls SDL_PollEvent at the
// top of each iteration.  Qt's event loop is framework-managed (app.exec()),
// so engine services are bridged into it via two peer classes:
//
//   EngineInterface  — pure C++ engine services (networking, game loop, session
//                      lifecycle).  Constructed first, destroyed last.  Has no
//                      knowledge of Qt.
//
//   QtAppInterface   — QObject layer that wires engine events into the Qt event
//                      loop via EngineEventBridge and exposes screen controllers
//                      to QML.  Depends on EngineInterface, not the reverse.
//
// main() handles only render-pipeline setup (which must happen before
// QGuiApplication) and coordinates the two interfaces.

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

    // --- Render pipeline (must be primed before QML engine) -------------------

    constexpr int render_w = 256 * 4;
    constexpr int render_h = 192 * 4;

    StateBuffer state_buffer;
    RenderBridge::instance().setStateBuffer(&state_buffer, render_w, render_h);
    Log::debug("[main] RenderBridge primed ({}x{})", render_w, render_h);

    auto gpu_backend = create_qt_render_backend();
    MediaManager::instance().assets().set_shader_backend(gpu_backend->backendName());
    QtDebugContext::instance().backend_name = gpu_backend->backendName();
    Log::debug("[main] shader backend set to '{}'", gpu_backend->backendName());

    // --- Engine + Qt interface (peers) ----------------------------------------

    EngineInterface engine(state_buffer);

    auto& qt_iface = QtAppInterface::instance();
    qt_iface.init(engine);

    if (!qt_iface.setup_qml()) {
        qt_iface.stop();
        engine.stop();
        MediaManager::instance().shutdown();
        return -1;
    }

    // Kick off the server-list fetch now that HTTP pool is ready.
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

    int result = app.exec();

    // --- Shutdown (reverse construction order) --------------------------------

    Log::info("[main] event loop exited (code {}), shutting down", result);
    qt_iface.stop();
    engine.stop();
    MediaManager::instance().shutdown();
    Log::info("[main] shutdown complete");

    return result;
}
