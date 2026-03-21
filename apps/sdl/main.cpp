// Engine
#include "asset/AssetLibrary.h"
#include "asset/MediaManager.h"
#include "event/EventManager.h"
#include "event/ServerListEvent.h"
#include "game/GameThread.h"
#include "game/ServerList.h"
#include "net/NetworkThread.h"
#include "render/RenderManager.h"
#include "render/StateBuffer.h"
#include "ui/UIManager.h"
#include "utils/Log.h"

// Plugins — create_renderer() is defined in whichever render plugin is linked.
#include "ao/ao_plugin.h"
#include "ao/ui/screens/ServerListScreen.h"
#include "render/IRenderer.h"

// App — create_gpu_backend() is defined in whichever backend source is linked.
#include "render/IGPUBackend.h"
#include "ui/DebugContext.h"
#include "ui/ImGuiUIRenderer.h"
#include "SDLGameWindow.h"
#include "ui/LogBuffer.h"

#include "asset/MountHttp.h"
#include "asset/MountManager.h"
#include "event/AssetUrlEvent.h"
#include "net/HttpPool.h"

#include <csignal>
#include <cstdlib>
#include <filesystem>

// Provided by the linked render plugin (aorender_gl or aorender_metal).
std::unique_ptr<IRenderer> create_renderer(int width, int height);

int main(int argc, char* argv[]) {
    // Ignore SIGPIPE so that writing to a closed socket returns EPIPE
    // instead of killing the process. Without this, a server disconnect
    // followed by a write() silently terminates the app on macOS/Linux.
    // The EPIPE error is then surfaced as an exception by the socket layer.
#ifndef _WIN32
    std::signal(SIGPIPE, SIG_IGN);
#endif
    LogBuffer::instance(); // Install log sink before anything logs
    MediaManager::instance().init(std::filesystem::path(std::getenv("HOME")) / "Documents" / "AO2" / "base");

    // HTTP thread pool — used for all HTTP downloads
    HttpPool http_pool(2);
    http_pool.get("http://servers.aceattorneyonline.com", "/servers", [](HttpResponse resp) {
        if (resp.status == 200) {
            ServerList svlist(resp.body);
            EventManager::instance().get_channel<ServerListEvent>().publish(ServerListEvent(svlist));
        } else {
            Log::log_print(ERR, "Failed to fetch server list: %s", resp.error.c_str());
        }
    });

    UIManager ui_mgr;
    ui_mgr.push_screen(std::make_unique<ServerListScreen>());
    SDLGameWindow game_window(ui_mgr, create_gpu_backend());

    // Workaround for Qt Creator
    setvbuf(stdout, NULL, _IONBF, 0);

    StateBuffer buffer;

    // Protocol plugin — swap this line to change protocols
    auto protocol = ao::create_protocol();
    NetworkThread net_thread(*protocol);

    // Render backend — scaled internal resolution (256x192 base, 4:3 aspect)
    auto& debug_ctx = DebugContext::instance();
    int render_w = DebugContext::BASE_W * debug_ctx.internal_scale.load();
    int render_h = DebugContext::BASE_H * debug_ctx.internal_scale.load();
    RenderManager renderer(buffer, create_renderer(render_w, render_h));
    MediaManager::instance().assets().set_shader_backend(renderer.get_renderer().backend_name());

    // Scene presenter — swap this to change game logic
    auto presenter = ao::create_presenter();
    GameThread game_logic(buffer, *presenter);

    debug_ctx.game_thread = &game_logic;
    debug_ctx.presenter = presenter.get();

    // Poll HTTP responses and handle asset URL events on the main thread
    game_window.set_frame_callback([&http_pool]() {
        http_pool.poll();

        // When the server sends an asset URL (ASS packet), create an HTTP mount
        auto& asset_ch = EventManager::instance().get_channel<AssetUrlEvent>();
        while (auto ev = asset_ch.get_event()) {
            auto mount = std::make_unique<MountHttp>(ev->url(), http_pool);
            MediaManager::instance().mounts_ref().add_mount(std::move(mount));
            Log::log_print(INFO, "Added HTTP asset mount: %s", ev->url().c_str());
        }
    });

    // Kick off the render loop with ImGui backend
    ImGuiUIRenderer ui_renderer;
    Log::log_print(INFO, "main: entering render loop");
    game_window.start_loop(renderer, ui_renderer);
    Log::log_print(DEBUG, "main: render loop exited");

    net_thread.stop();
    Log::log_print(DEBUG, "main: network thread stopped");
    game_logic.stop();
    Log::log_print(DEBUG, "main: game thread stopped");

    MediaManager::instance().shutdown();
    Log::log_print(INFO, "main: shutdown complete");

    return 0;
}
