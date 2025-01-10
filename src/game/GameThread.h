#pragma once

#include "render/StateBuffer.h"

#include <atomic>
#include <thread>

class GameThread {
  public:
    GameThread(StateBuffer& render_buffer);
    void stop();

  private:
    void game_loop();
    void tick(uint64_t t);
    void render(RenderState new_state);

    std::atomic<bool> running;

    StateBuffer& render_buffer;
    std::thread tick_thread;
};
