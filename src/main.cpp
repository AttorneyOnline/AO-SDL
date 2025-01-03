#include <chrono>
#include <cmath>
#include <thread>

#include "event/EventManager.h"
#include "game/GameThread.h"
#include "net/NetworkThread.h"
#include "net/WebSocket.h"
#include "utils/Log.h"
#include "video/GameWindow.h"

int main(int argc, char* argv[]) {
    // Set up the EventChannels and give them to the manager
    std::unique_ptr<EventChannel<UIEvent>> ui_ev_channel = std::make_unique<EventChannel<UIEvent>>();
    std::unique_ptr<EventChannel<ChatEvent>> chat_ev_channel = std::make_unique<EventChannel<ChatEvent>>();

    EventManager& ev_mgr = EventManager::get_instance();
    ev_mgr.set_ui_channel(std::move(ui_ev_channel));
    ev_mgr.set_chat_channel(std::move(chat_ev_channel));

    UIManager ui_mgr;
    GameWindow game_window(ui_mgr);

    // Workaround for Qt Creator
    setbuf(stdout, NULL);

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
