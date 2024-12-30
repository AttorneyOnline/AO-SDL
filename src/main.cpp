#include <chrono>
#include <cmath>
#include <thread>

#include "game/GameThread.h"
#include "video/GameWindow.h"

int main(int argc, char* argv[]) {
    GameWindow game_window;

    // Workaround for Qt Creator
    setbuf(stdout, NULL);

    // Initialize StateBuffer
    StateBuffer buffer;

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
