#include "ao/net/AOClient.h"
#include "asset/MediaManager.h"
#include "event/EventManager.h"
#include "event/ServerListEvent.h"
#include "game/CourtroomPresenter.h"
#include "game/GameThread.h"
#include "game/ServerList.h"
#include "net/NetworkThread.h"
#include "video/GameWindow.h"

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
    GameWindow game_window(ui_mgr);

    // Workaround for Qt Creator
    setvbuf(stdout, NULL, _IONBF, 0);

    // Initialize StateBuffer
    StateBuffer buffer;

    AOClient ao_client;
    NetworkThread net_thread(ao_client);

    // Instantiate renderer
    RenderManager renderer(buffer);

    // Start game logic thread
    CourtroomPresenter presenter;
    GameThread game_logic(buffer, presenter);

    // Kick off the render loop
    game_window.start_loop(renderer);

    // todo: cleanup memory
    game_logic.stop();

    return 0;
}
