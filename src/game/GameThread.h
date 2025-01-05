#pragma once

#include <atomic>
#include <thread>

#include "render/StateBuffer.h"

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
