#include "StateBuffer.h"

StateBuffer::StateBuffer() : stale(false) {
    RenderState initial_buf;

    presenting = new RenderState(initial_buf);
    ready = new RenderState(initial_buf);
    preparing = new RenderState(initial_buf);
}

StateBuffer::StateBuffer(RenderState initial_buf) : stale(false) {
    // Copy initial_buf by value into all three buffers here
    presenting = new RenderState(initial_buf);
    ready = new RenderState(initial_buf);
    preparing = new RenderState(initial_buf);
}

const RenderState* StateBuffer::get_consumer_buf() {
    return presenting;
}

RenderState* StateBuffer::get_producer_buf() {
    return preparing;
}

void StateBuffer::present() {
    const std::lock_guard<std::mutex> lock(swap_mutex);

    std::swap(preparing, ready);
    stale = true;
}

void StateBuffer::update() {
    const std::lock_guard<std::mutex> lock(swap_mutex);

    if (stale) {
        std::swap(presenting, ready);
        stale = false;
    }
}
