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

#include "httplib.h"

#include <cstdlib>
#include <filesystem>

// Provided by the linked render plugin (aorender_gl or aorender_metal).
std::unique_ptr<IRenderer> create_renderer(int width, int height);

int main(int argc, char* argv[]) {
    MediaManager::instance().init(std::filesystem::path(std::getenv("HOME")) / "Documents" / "AO2" / "base");

    // todo: kick off another thread to do this
    httplib::Client cli("http://servers.aceattorneyonline.com");
    auto res = cli.Get("/servers");
    if (res && res->status == 200) {
        ServerList svlist(res->body);
        EventManager::instance().get_channel<ServerListEvent>().publish(ServerListEvent(svlist));
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

    // Render backend — AO2 viewport is 256x192
    RenderManager renderer(buffer, create_renderer(256, 192));
    MediaManager::instance().assets().set_shader_backend(renderer.get_renderer().backend_name());

    // Scene presenter — swap this to change game logic
    auto presenter = ao::create_presenter();
    GameThread game_logic(buffer, *presenter);

    DebugContext::instance().game_thread = &game_logic;
    DebugContext::instance().presenter = presenter.get();

    // Kick off the render loop with ImGui backend
    ImGuiUIRenderer ui_renderer;
    game_window.start_loop(renderer, ui_renderer);

    net_thread.stop();
    game_logic.stop();

    MediaManager::instance().shutdown();

    return 0;
}
