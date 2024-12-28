#include <chrono>
#include <cmath>
#include <thread>

#include "video/GameWindow.h"
#include "game/GameThread.h"


int main(int argc, char* argv[]) {
    GameWindow game_window;

    // Set up initial RenderState


    // Initialize StateBuffer
    StateBuffer buffer;
    RenderManager renderer(buffer);
    
    GameThread game_logic(buffer);

    game_window.start_loop(renderer);

    // todo: cleanup

    return 0;
}
