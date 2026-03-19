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

// Plugins
#include "ao/ao_plugin.h"
#include "render/gl/GLRenderer.h"

// App
#include "ImGuiUIRenderer.h"
#include "SDLGameWindow.h"

#include "httplib.h"

int main(int argc, char* argv[]) {
    MediaManager::instance().init("G:/AO2/base");

    // todo: kick off another thread to do this
    httplib::Client cli("http://servers.aceattorneyonline.com");
    auto res = cli.Get("/servers");
    if (res && res->status == 200) {
        ServerList svlist(res->body);
        EventManager::instance().get_channel<ServerListEvent>().publish(ServerListEvent(svlist));
    }

    UIManager ui_mgr;
    SDLGameWindow game_window(ui_mgr);

    // Workaround for Qt Creator
    setvbuf(stdout, NULL, _IONBF, 0);

    // Initialize StateBuffer
    StateBuffer buffer;

    // Protocol plugin — swap this line to change protocols
    auto protocol = ao::create_protocol();
    NetworkThread net_thread(*protocol);

    // Load shaders from the asset system
    auto& assets = MediaManager::instance().assets();
    auto vert_data = assets.raw("shaders/vertex.glsl");
    auto frag_data = assets.raw("shaders/fragment.glsl");
    if (!vert_data || !frag_data) {
        Log::log_print(FATAL, "Failed to load shaders from asset system");
        return 1;
    }
    std::string vert_source(vert_data->begin(), vert_data->end());
    std::string frag_source(frag_data->begin(), frag_data->end());

    // Render backend — AO2 viewport is 256x192
    RenderManager renderer(buffer, create_gl_renderer(vert_source, frag_source, 256, 192));

    // Scene presenter — swap this to change game logic
    auto presenter = ao::create_presenter();
    GameThread game_logic(buffer, *presenter);

    // Kick off the render loop with ImGui backend
    ImGuiUIRenderer ui_renderer;
    game_window.start_loop(renderer, ui_renderer);

    // todo: cleanup memory
    game_logic.stop();

    return 0;
}
