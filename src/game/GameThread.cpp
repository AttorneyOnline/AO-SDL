#include "GameThread.h"

GameThread::GameThread(StateBuffer& render_buffer, IScenePresenter& presenter)
    : running(true), render_buffer(render_buffer), presenter(presenter),
      tick_thread(&GameThread::game_loop, this) {
}

void GameThread::stop() {
    running = false;
    tick_thread.join();
}

void GameThread::game_loop() {
    uint64_t t = 0;

    while (running) {
        RenderState state = presenter.tick(t);

        RenderState* buf = render_buffer.get_producer_buf();
        *buf = state;
        render_buffer.present();

        t++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
