#pragma once

#include "IScenePresenter.h"
#include "render/StateBuffer.h"

#include <atomic>
#include <thread>

class GameThread {
  public:
    GameThread(StateBuffer& render_buffer, IScenePresenter& presenter);
    void stop();

  private:
    void game_loop();

    std::atomic<bool> running;
    StateBuffer& render_buffer;
    IScenePresenter& presenter;
    std::thread tick_thread;
};
