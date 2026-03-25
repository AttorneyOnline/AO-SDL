// Engine
#include "asset/AssetLibrary.h"
#include "asset/MediaManager.h"
#include "audio/AudioThread.h"
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
#include "SDLGameWindow.h"
#include "audio/SDLAudioDevice.h"
#include "render/IGPUBackend.h"
#include "ui/DebugContext.h"
#include "ui/ImGuiUIRenderer.h"
#include "ui/LogBuffer.h"

#include "event/AssetUrlEvent.h"
#include "event/SessionEndEvent.h"
#include "event/SessionStartEvent.h"
#include "game/Session.h"
#include "net/HttpPool.h"

#include <csignal>
#include <cstdlib>
#include <filesystem>

// Tell Windows GPU drivers to prefer the discrete (high-performance) GPU when
// the system has both integrated and discrete graphics (common on AMD APU +
// dGPU and NVIDIA Optimus laptops). Without these exports the OpenGL ICD may
// not load at all, leaving only the GDI Generic GL 1.1 software renderer.
#ifdef _WIN32
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

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

    // Audio device — SDL-based, opened before game logic starts
    SDLAudioDevice audio_device;
    audio_device.open();
    AudioThread audio_thread(audio_device, MediaManager::instance().mounts_ref());

    // Scene presenter — swap this to change game logic
    auto presenter = ao::create_presenter();
    GameThread game_logic(buffer, *presenter);

    debug_ctx.game_thread = &game_logic;
    debug_ctx.presenter = presenter.get();
    debug_ctx.audio_device = &audio_device;

    // Poll HTTP responses and manage Session lifecycle on the main thread.
    std::unique_ptr<Session> active_session;

    game_window.set_frame_callback([&http_pool, &active_session]() {
        http_pool.poll();

        // Session start: network thread connected to a server.
        // Add the global fallback mount at low priority so server-specific
        // mounts (added on ASS packets) are always searched first.
        auto& start_ch = EventManager::instance().get_channel<SessionStartEvent>();
        if (start_ch.get_event()) {
            active_session =
                std::make_unique<Session>(MediaManager::instance().mounts_ref(), MediaManager::instance().assets());
            active_session->add_http_mount("https://attorneyoffline.de/base/", http_pool, 300);
        }

        // When the server sends an asset URL (ASS packet), add its HTTP mount
        // at default priority (200), which is higher than the fallback (300).
        auto& asset_ch = EventManager::instance().get_channel<AssetUrlEvent>();
        while (auto ev = asset_ch.get_event()) {
            if (active_session) {
                active_session->add_http_mount(ev->url(), http_pool);
            }
        }

        // Session end: network thread disconnected — destructor handles all cleanup
        auto& end_ch = EventManager::instance().get_channel<SessionEndEvent>();
        if (end_ch.get_event()) {
            active_session.reset();
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
    audio_thread.stop();
    audio_device.close();
    Log::log_print(DEBUG, "main: audio thread stopped");

    http_pool.stop();
    Log::log_print(DEBUG, "main: HTTP pool stopped");
    MediaManager::instance().shutdown();
    Log::log_print(INFO, "main: shutdown complete");

    return 0;
}
