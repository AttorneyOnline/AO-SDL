#ifndef GAMETHREAD_H
#define GAMETHREAD_H

#include <thread>

#include <StateBuffer.h>

class GameThread {
  public:
    GameThread(StateBuffer& render_buffer);

  private:
    void tick();

    StateBuffer& render_buffer;
    std::thread tick_thread;
};

#endif