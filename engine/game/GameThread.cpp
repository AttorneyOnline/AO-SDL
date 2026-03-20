#include "game/GameThread.h"

GameThread::GameThread(StateBuffer& render_buffer, IScenePresenter& presenter)
    : running(true), render_buffer(render_buffer), presenter(presenter), tick_thread(&GameThread::game_loop, this) {
}

void GameThread::stop() {
    running = false;
    tick_thread.join();
}

void GameThread::game_loop() {
    auto last = std::chrono::steady_clock::now();

    while (running) {
        auto now = std::chrono::steady_clock::now();
        auto delta_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
        last = now;

        RenderState state = presenter.tick(static_cast<uint64_t>(delta_ms));

        RenderState* buf = render_buffer.get_producer_buf();
        *buf = state;
        render_buffer.present();

        // ~60 Hz for smooth text and animation timing
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}
