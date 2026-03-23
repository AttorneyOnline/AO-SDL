// Qt
#include <QGuiApplication>
#include <QQuickView>

// Engine
#include "asset/MediaManager.h"
#include "event/EventManager.h"
#include "event/ServerListEvent.h"
#include "utils/Log.h"

// Plugins
#include "ao/ui/screens/ServerListScreen.h"

// App — create_gpu_backend() is defined in whichever backend source is linked.
#include "QtGameWindow.h"
#include "renderer/IGPUBackend.h"

#include "net/HttpPool.h"

int main(int argc, char* argv[]) {
    Log::log_print(DEBUG, "Starting AOQML.exe");
    // Ignore SIGPIPE so that writing to a closed socket returns EPIPE
    // instead of killing the process. Without this, a server disconnect
    // followed by a write() silently terminates the app on macOS/Linux.
    // The EPIPE error is then surfaced as an exception by the socket layer.
#ifndef _WIN32
    std::signal(SIGPIPE, SIG_IGN);
#endif

    // HTTP thread pool — used for all HTTP downloads.
    // Each thread keeps a persistent connection per host (HTTP keep-alive),
    // so a small pool saturates the link without overwhelming the server.
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

    // Mount local base/ directory relative to the binary
    {
        auto exe_dir = std::filesystem::path(argv[0]).parent_path();
        auto base_dir = exe_dir / "base";
        if (std::filesystem::is_directory(base_dir)) {
            MediaManager::instance().init(base_dir);
            Log::log_print(INFO, "Mounted local content: %s", base_dir.c_str());
        }
    }

    // Workaround for Qt Creator
    setvbuf(stdout, NULL, _IONBF, 0);

    IGPUBackend::pre_init();
    QGuiApplication app(argc, argv);

    UIManager ui_mgr;
    ui_mgr.push_screen(std::make_unique<ServerListScreen>());
    QtGameWindow game_window(ui_mgr, create_gpu_backend());

    return app.exec();
}
