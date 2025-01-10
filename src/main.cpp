#include "event/EventManager.h"
#include "event/ServerListEvent.h"
#include "game/GameThread.h"
#include "game/ServerList.h"
#include "net/NetworkThread.h"
#include "net/WebSocket.h"
#include "video/GameWindow.h"

#include "httplib.h"

int main(int argc, char* argv[]) {
    // todo: kick off another thread to do this

    httplib::Client cli("http://servers.aceattorneyonline.com");
    auto res = cli.Get("/servers");

    if (res->status == 200) {
        ServerList svlist(res->body);
        EventManager::instance().get_channel<ServerListEvent>().publish(ServerListEvent(svlist, EventTarget::UI));
    }

    UIManager ui_mgr;
    GameWindow game_window(ui_mgr);

    // Workaround for Qt Creator
    setvbuf(stdout, NULL, _IONBF, 0);

    // Initialize StateBuffer
    StateBuffer buffer;

    WebSocket sock("securevanilla.aceattorneyonline.com", 2095);
    // WebSocket sock("localhost", 27017);
    NetworkThread net_thread(sock);

    // Instantiate renderer
    RenderManager renderer(buffer);

    // Start game logic thread
    GameThread game_logic(buffer);

    // Kick off the render loop
    game_window.start_loop(renderer);

    // todo: cleanup memory
    game_logic.stop();

    return 0;
}
