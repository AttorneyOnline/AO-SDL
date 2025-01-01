#include <chrono>
#include <cmath>
#include <thread>

#include "game/GameThread.h"
#include "net/WebSocket.h"
#include "utils/Log.h"
#include "video/GameWindow.h"

int main(int argc, char* argv[]) {
    // GameWindow game_window;

    // Workaround for Qt Creator
    setbuf(stdout, NULL);

    // Initialize StateBuffer
    StateBuffer buffer;

    WebSocket sock("localhost", 27017);
    sock.connect();

    std::vector<WebSocket::WebSocketFrame> msgs;
    while (msgs.size() < 1) {
        msgs = sock.read();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::string msgstr(msgs[0].data.begin(), msgs[0].data.end());
    Log::log_print(DEBUG, "%s", msgstr.c_str());

    msgs.clear();
    std::string resp = "HI#bullshit#%";
    std::vector<uint8_t> respbuf(resp.begin(), resp.end());
    sock.write(respbuf);

    while (true) {
        while (msgs.size() < 1) {
            msgs = sock.read();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        msgstr = std::string(msgs[0].data.begin(), msgs[0].data.end());
        Log::log_print(DEBUG, "%s", msgstr.c_str());
        msgs.clear();
    }

    return 0;

    // Instantiate renderer
    RenderManager renderer(buffer);

    // Start game logic thread
    GameThread game_logic(buffer);

    // Kick off the render loop
    // game_window.start_loop(renderer);

    // todo: cleanup memory
    game_logic.stop();

    return 0;
}
