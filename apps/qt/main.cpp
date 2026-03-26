// Engine
#include "asset/MediaManager.h"
#include "audio/AudioThread.h"
#include "event/AssetUrlEvent.h"
#include "event/EventManager.h"
#include "event/ServerListEvent.h"
#include "event/SessionEndEvent.h"
#include "event/SessionStartEvent.h"
#include "game/GameThread.h"
#include "game/ServerList.h"
#include "game/Session.h"
#include "net/HttpPool.h"
#include "net/NetworkThread.h"
#include "render/StateBuffer.h"
#include "ui/UIManager.h"
#include "utils/Log.h"

// Plugin
#include "ao/ao_plugin.h"
#include "ao/ui/screens/CharSelectScreen.h"
#include "ao/ui/screens/CourtroomScreen.h"
#include "ao/ui/screens/ServerListScreen.h"

// Qt app
#include "AppContext.h"
#include "EngineEventBridge.h"
#include "QtDebugContext.h"
#include "QtGameWindow.h"
#include "audio/NullAudioDevice.h"
#include "ui/QtUIRenderer.h"
#include "ui/controllers/CharSelectController.h"
#include "ui/controllers/CourtroomController.h"
#include "ui/controllers/ServerListController.h"

#include <QGuiApplication>
#include <csignal>
#include <cstdio>
#include <filesystem>
#include <memory>

int main(int argc, char* argv[]) {
#ifndef _WIN32
    // Ignore SIGPIPE so that writing to a closed socket returns EPIPE
    // instead of killing the process. EPIPE is then surfaced as an exception
    // by the socket layer.
    std::signal(SIGPIPE, SIG_IGN);
#endif

    QGuiApplication app(argc, argv);
    app.setApplicationName("Attorney Online");
    app.setOrganizationName("AceAttorneyOnline");

    // Workaround for output buffering when running inside Qt Creator.
    setvbuf(stdout, nullptr, _IONBF, 0);

    // HTTP thread pool — used for all HTTP downloads.
    // Each thread keeps a persistent connection per host (HTTP keep-alive).
    HttpPool http_pool(8);
    http_pool.get("http://servers.aceattorneyonline.com", "/servers",
                  [](HttpResponse resp) {
                      if (resp.status == 200) {
                          ServerList svlist(resp.body);
                          EventManager::instance()
                              .get_channel<ServerListEvent>()
                              .publish(ServerListEvent(svlist));
                      } else {
                          Log::log_print(ERR, "Failed to fetch server list: %s",
                                         resp.error.c_str());
                      }
                  });

    // Mount local base/ directory relative to the binary.
    {
        auto exe_dir  = std::filesystem::path(argv[0]).parent_path();
        auto base_dir = exe_dir / "base";
        if (std::filesystem::is_directory(base_dir)) {
            MediaManager::instance().init(base_dir);
            Log::log_print(INFO, "Mounted local content: %s", base_dir.c_str());
        }
    }

    UIManager ui_mgr;
    ui_mgr.push_screen(std::make_unique<ServerListScreen>());

    // Triple-buffered state shared between GameThread (writer) and the
    // render path in QtGameWindow (reader via SceneTextureItem).
    StateBuffer buffer;

    // Protocol plugin — swap this line to change protocols.
    auto protocol = ao::create_protocol();
    NetworkThread net_thread(*protocol);

    // Scene presenter — swap this to change game logic.
    auto presenter = ao::create_presenter();
    GameThread game_logic(buffer, *presenter);

    // Expose engine subsystems to the debug overlay.
    auto& dbg       = QtDebugContext::instance();
    dbg.game_thread = &game_logic;
    dbg.presenter   = presenter.get();

    // QtGameWindow creates the IRenderer internally after the graphics context
    // is ready, then exposes it to QML via RenderBridge.
    const int render_w = QtDebugContext::BASE_W * dbg.internal_scale.load();
    const int render_h = QtDebugContext::BASE_H * dbg.internal_scale.load();

    NullAudioDevice audio_device;
    audio_device.open();
    AudioThread audio_thread(audio_device, MediaManager::instance().mounts_ref());

    // Qt screen controllers — one per Screen subclass; owned here, registered
    // with QtUIRenderer so sync() is called on every event-loop tick.
    ServerListController sl_ctrl;
    CharSelectController cs_ctrl;
    CourtroomController  cr_ctrl;

    QtUIRenderer ui_renderer(ui_mgr);
    ui_renderer.registerController(ServerListScreen::ID, &sl_ctrl);
    ui_renderer.registerController(CharSelectScreen::ID, &cs_ctrl);
    ui_renderer.registerController(CourtroomScreen::ID,  &cr_ctrl);

    // Populate before QtGameWindow is constructed so QML bindings on "app.*"
    // evaluate against valid pointers when the component is first created.
    AppContext::instance().setControllers(&sl_ctrl, &cs_ctrl, &cr_ctrl);

    // EngineEventBridge — drains engine event channels on every Qt event loop
    // wake-up, replacing the SDL frame_callback_ + per-frame poll pattern.
    EngineEventBridge event_bridge;

    event_bridge.addChannel([&http_pool] {
        http_pool.poll();
    });

    event_bridge.addChannel([&ui_mgr] {
        ui_mgr.handle_events();
    });

    // Sync Qt models from engine screen state, must run after handle_events().
    event_bridge.addChannel([&ui_renderer] {
        ui_renderer.update();
    });

    std::unique_ptr<Session> active_session;

    event_bridge.addChannel([&active_session, &http_pool] {
        auto& ch = EventManager::instance().get_channel<SessionStartEvent>();
        if (ch.get_event()) {
            active_session = std::make_unique<Session>(
                MediaManager::instance().mounts_ref(),
                MediaManager::instance().assets());
            // Fallback HTTP mount at low priority; server-specific mounts
            // (added on ASS packets) are inserted at higher priority.
            active_session->add_http_mount(
                "https://attorneyoffline.de/base/", http_pool, 300);
        }
    });

    event_bridge.addChannel([&active_session, &http_pool] {
        auto& ch = EventManager::instance().get_channel<AssetUrlEvent>();
        while (auto ev = ch.get_event()) {
            if (active_session)
                active_session->add_http_mount(ev->url(), http_pool);
        }
    });

    event_bridge.addChannel([&active_session] {
        auto& ch = EventManager::instance().get_channel<SessionEndEvent>();
        if (ch.get_event())
            active_session.reset();
    });

    QtGameWindow game_window(ui_mgr, buffer, render_w, render_h);

    // Forward navigation signals from any controller to the window.
    QObject::connect(&ui_renderer, &QtUIRenderer::navActionRequested,
                     &game_window, &QtGameWindow::onNavAction);

    game_window.show();

    // Connect to the Qt event loop dispatcher after the window is up.
    event_bridge.start();

    Log::log_print(INFO, "main: entering Qt event loop");
    const int result = app.exec();
    Log::log_print(DEBUG, "main: Qt event loop exited");

    event_bridge.stop();

    net_thread.stop();
    Log::log_print(DEBUG, "main: network thread stopped");

    game_logic.stop();
    Log::log_print(DEBUG, "main: game thread stopped");

    audio_thread.stop();
    audio_device.close();
    Log::log_print(DEBUG, "main: audio stopped");

    http_pool.stop();
    Log::log_print(DEBUG, "main: HTTP pool stopped");

    MediaManager::instance().shutdown();
    Log::log_print(INFO, "main: shutdown complete");

    return result;
}
