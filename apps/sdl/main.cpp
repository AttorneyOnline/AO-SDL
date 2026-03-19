// Engine
#include "asset/MediaManager.h"
#include "event/EventManager.h"
#include "event/ServerListEvent.h"
#include "game/CourtroomPresenter.h"
#include "game/GameThread.h"
#include "game/ServerList.h"
#include "net/NetworkThread.h"
#include "render/RenderManager.h"
#include "render/StateBuffer.h"
#include "ui/UIManager.h"

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

    // Render backend — swap this to change renderers
    RenderManager renderer(buffer, create_gl_renderer());

    // Start game logic thread
    CourtroomPresenter presenter;
    GameThread game_logic(buffer, presenter);

    // Kick off the render loop with ImGui backend
    ImGuiUIRenderer ui_renderer;
    game_window.start_loop(renderer, ui_renderer);

    // todo: cleanup memory
    game_logic.stop();

    return 0;
}
