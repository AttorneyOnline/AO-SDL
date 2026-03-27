#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>

#include "AppContext.h"
#include "EngineEventBridge.h"
#include "asset/MediaManager.h"
#include "event/AssetUrlEvent.h"
#include "event/EventManager.h"
#include "event/ServerListEvent.h"
#include "event/SessionEndEvent.h"
#include "event/SessionStartEvent.h"
#include "game/ServerList.h"
#include "game/GameThread.h"
#include "game/Session.h"
#include "net/HttpPool.h"
#include "net/NetworkThread.h"
#include "render/RenderBridge.h"
#include "render/StateBuffer.h"
#include "ui/UIManager.h"
#include "ui/controllers/CharSelectController.h"
#include "ui/controllers/CourtroomController.h"
#include "ui/controllers/DummyController.h"
#include "ui/controllers/ServerListController.h"

#include "ao/ao_plugin.h"
#include "ao/ui/screens/ServerListScreen.h"

int main(int argc, char* argv[]) {
#if !defined(Q_OS_APPLE)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#endif

    QGuiApplication app(argc, argv);
    app.setApplicationName("Attorney Online");

    // Internal render resolution: 256×192 base at 4× scale → 1024×768.
    constexpr int renderW = 256 * 4;
    constexpr int renderH = 192 * 4;

    // Prime the bridge before the QML engine is created.  SceneTextureItem's
    // initGL() (render thread) reads these values to construct its RenderManager.
    StateBuffer stateBuffer;
    RenderBridge::instance().setStateBuffer(&stateBuffer, renderW, renderH);

    // AO2 scene presenter drives the courtroom renderer.
    auto   presenter = ao::create_presenter();
    GameThread gameThread(stateBuffer, *presenter);

    // --- Navigation -----------------------------------------------------------

    UIManager uiMgr;
    uiMgr.push_screen(std::make_unique<ServerListScreen>());

    // --- Controllers (construction order matters: crCtrl before csCtrl) ------

    CourtroomController  crController(uiMgr);
    CharSelectController csController(uiMgr, crController);
    ServerListController slController(uiMgr);
    DummyController      dummyController(uiMgr);

    AppContext::instance().setUIManager(&uiMgr);
    AppContext::instance().setControllers(&slController, &csController, &crController);
    AppContext::instance().setDummyController(&dummyController);

    // Initialise QML context before the engine loads Main.qml.
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("app", &AppContext::instance());

    // --- Network + HTTP -------------------------------------------------------

    HttpPool httpPool(4);

    // Kick off the server-list fetch immediately.
    httpPool.get(
        "http://servers.aceattorneyonline.com", "/servers",
        [](HttpResponse resp) {
            if (resp.status == 200) {
                ServerList svlist(resp.body);
                EventManager::instance()
                    .get_channel<ServerListEvent>()
                    .publish(ServerListEvent(std::move(svlist)));
            }
        });

    auto protocol = ao::create_protocol();
    NetworkThread netThread(*protocol);

    // --- Event bridge ---------------------------------------------------------
    //
    // All drains run on the main thread, in registration order, each time the
    // Qt event loop wakes up.

    std::unique_ptr<Session> activeSession;

    EngineEventBridge bridge;

    // 1. Deliver HTTP callbacks to their originators on the main thread.
    bridge.addChannel([&] { httpPool.poll(); });

    // 2. Session lifecycle.
    bridge.addChannel([&] {
        if (EventManager::instance().get_channel<SessionStartEvent>().get_event()) {
            activeSession = std::make_unique<Session>(
                MediaManager::instance().mounts_ref(),
                MediaManager::instance().assets());
            activeSession->add_http_mount(
                "https://attorneyoffline.de/base/", httpPool, 300);
        }

        auto& assetCh = EventManager::instance().get_channel<AssetUrlEvent>();
        while (auto ev = assetCh.get_event()) {
            if (activeSession)
                activeSession->add_http_mount(ev->url(), httpPool);
        }

        if (EventManager::instance().get_channel<SessionEndEvent>().get_event())
            activeSession.reset();
    });

    // 3. Drain all controllers — each pulls only its own EventChannels.
    bridge.addChannel([&] {
        slController.drain();
        csController.drain();
        crController.drain();
        // DummyController::drain() is a no-op; skip for clarity.
    });

    // 4. Propagate UIManager's active screen id to QML for Loader navigation.
    bridge.addChannel([&] { AppContext::instance().syncCurrentScreenId(); });

    bridge.start();

    // --- QML ------------------------------------------------------------------

    engine.loadFromModule("AO", "Main");
    if (engine.rootObjects().isEmpty()) {
        bridge.stop();
        netThread.stop();
        gameThread.stop();
        httpPool.stop();
        MediaManager::instance().shutdown();
        return -1;
    }

    int result = app.exec();

    // --- Shutdown (reverse construction order) --------------------------------

    bridge.stop();
    netThread.stop();
    gameThread.stop();
    httpPool.stop();
    MediaManager::instance().shutdown();

    return result;
}
