#pragma once

#include "RenderState.h"

#include <atomic>
#include <mutex>

class StateBuffer {
  public:
    StateBuffer();
    StateBuffer(RenderState initial_buf);

    RenderState* get_producer_buf();
    const RenderState* get_consumer_buf();

    void present();
    void update();

  private:
    RenderState* presenting;
    RenderState* ready;
    RenderState* preparing;

    std::atomic<bool> stale;
    std::mutex swap_mutex;
};
