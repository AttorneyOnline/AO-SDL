// Qt
#include <QGuiApplication>

// Engine
#include "asset/AssetLibrary.h"
#include "asset/MediaManager.h"
#include "asset/MountHttp.h"
#include "asset/MountManager.h"
#include "event/AssetUrlEvent.h"
#include "event/EventManager.h"
#include "event/ServerListEvent.h"
#include "game/GameThread.h"
#include "game/ServerList.h"
#include "net/HttpPool.h"
#include "net/NetworkThread.h"
#include "render/IRenderer.h"
#include "render/RenderManager.h"
#include "render/StateBuffer.h"
#include "ui/UIManager.h"
#include "utils/Log.h"

// Plugins
#include "ao/ao_plugin.h"
#include "ao/ui/screens/ServerListScreen.h"

// App
#include "QmlUIBridge.h"
#include "qtgamewindow.h"
#include "renderer/IGPUBackend.h"

#include <csignal>
#include <filesystem>

// Provided by the linked render plugin (aorender_gl or aorender_metal).
std::unique_ptr<IRenderer> create_renderer(int width, int height);

int main(int argc, char* argv[]) {
#ifndef _WIN32
    std::signal(SIGPIPE, SIG_IGN);
#endif
    Log::log_print(INFO, "Starting AOQML");

    // Workaround for Qt Creator — unbuffered stdout for live log output.
    setvbuf(stdout, NULL, _IONBF, 0);

    // HTTP thread pool — 8 threads, persistent connections per host.
    HttpPool http_pool(8);
    http_pool.get("http://servers.aceattorneyonline.com", "/servers", [](HttpResponse resp) {
        if (resp.status == 200) {
            ServerList svlist(resp.body);
            EventManager::instance().get_channel<ServerListEvent>().publish(ServerListEvent(svlist));
        }
        else {
            Log::log_print(ERR, "Failed to fetch server list: %s", resp.error.c_str());
        }
    });

    // Mount local base/ directory relative to the binary.
    {
        auto exe_dir = std::filesystem::path(argv[0]).parent_path();
        auto base_dir = exe_dir / "base";
        if (std::filesystem::is_directory(base_dir)) {
            MediaManager::instance().init(base_dir);
            Log::log_print(INFO, "Mounted local content: %s", base_dir.c_str());
        }
    }

    // Qt & GPU setup — pre_init() must happen before QGuiApplication.
    IGPUBackend::pre_init();
    QGuiApplication app(argc, argv);

    // Screen stack & window.
    UIManager ui_mgr;
    ui_mgr.push_screen(std::make_unique<ServerListScreen>());
    QtGameWindow game_window(ui_mgr, create_gpu_backend());

    // Triple buffer for game thread → render thread communication.
    StateBuffer buffer;

    // Protocol + network thread.
    auto protocol = ao::create_protocol();
    NetworkThread net_thread(*protocol);

    // Scene presenter + game thread (~10 Hz tick).
    auto presenter = ao::create_presenter();
    GameThread game_logic(buffer, *presenter);

    // QML bridge — exposes screen state to the QML UI.
    QmlUIBridge qml_bridge(ui_mgr);

    // Per-frame callback: poll HTTP responses and mount asset URLs.
    game_window.set_frame_callback([&http_pool]() {
        http_pool.poll();

        auto& asset_ch = EventManager::instance().get_channel<AssetUrlEvent>();
        while (auto ev = asset_ch.get_event()) {
            auto mount = std::make_unique<MountHttp>(ev->url(), http_pool);
            MediaManager::instance().mounts_ref().add_mount(std::move(mount));
            Log::log_print(INFO, "Added HTTP asset mount: %s", ev->url().c_str());

            static bool default_mount_added = false;
            if (!default_mount_added) {
                default_mount_added = true;
                auto fallback = std::make_unique<MountHttp>("https://attorneyoffline.de/base/", http_pool);
                MediaManager::instance().mounts_ref().add_mount(std::move(fallback));
                Log::log_print(INFO, "Added fallback HTTP asset mount: https://attorneyoffline.de/base/");
            }
        }
    });

    // Renderer is created lazily inside sceneGraphInitialized (GL context needed).
    constexpr int INTERNAL_SCALE = 4;
    constexpr int RENDER_W = 256 * INTERNAL_SCALE;
    constexpr int RENDER_H = 192 * INTERNAL_SCALE;

    game_window.start_rendering(
        [&buffer]() -> std::unique_ptr<RenderManager> {
            return std::make_unique<RenderManager>(buffer, create_renderer(RENDER_W, RENDER_H));
        },
        qml_bridge, RENDER_W, RENDER_H);
    Log::log_print(INFO, "main: entering Qt event loop");

    int result = app.exec();

    // Shutdown — order matters: stop consumers before producers.
    Log::log_print(DEBUG, "main: Qt event loop exited");
    net_thread.stop();
    Log::log_print(DEBUG, "main: network thread stopped");
    game_logic.stop();
    Log::log_print(DEBUG, "main: game thread stopped");
    http_pool.stop();
    Log::log_print(DEBUG, "main: HTTP pool stopped");
    MediaManager::instance().shutdown();
    Log::log_print(INFO, "main: shutdown complete");

    return result;
}
