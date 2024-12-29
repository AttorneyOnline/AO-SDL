#ifndef GAMETHREAD_H
#define GAMETHREAD_H

#include <thread>

#include "render/StateBuffer.h"

class GameThread {
  public:
    GameThread(StateBuffer& render_buffer);

  private:
    void game_loop();
    void tick(uint64_t t);
    void render(RenderState new_state);

    StateBuffer& render_buffer;
    std::thread tick_thread;
};

#endif