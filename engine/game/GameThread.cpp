#include "game/GameThread.h"

GameThread::GameThread(StateBuffer& render_buffer, IScenePresenter& presenter)
    : running(true), last_tick_us_(0), tick_rate_hz_(0), render_buffer(render_buffer), presenter(presenter),
      tick_thread(&GameThread::game_loop, this) {
}

void GameThread::stop() {
    running = false;
    tick_thread.join();
}

void GameThread::game_loop() {
    presenter.init();

    auto last = std::chrono::steady_clock::now();
    int tick_count = 0;
    auto rate_start = std::chrono::steady_clock::now();

    while (running) {
        auto now = std::chrono::steady_clock::now();
        auto delta_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
        last = now;

        auto tick_start = std::chrono::steady_clock::now();
        RenderState state = presenter.tick(static_cast<uint64_t>(delta_ms));
        auto tick_end = std::chrono::steady_clock::now();
        last_tick_us_.store(
            static_cast<int>(std::chrono::duration_cast<std::chrono::microseconds>(tick_end - tick_start).count()),
            std::memory_order_relaxed);

        // Measure tick rate over 1-second windows
        tick_count++;
        auto rate_elapsed = std::chrono::duration<float>(tick_end - rate_start).count();
        if (rate_elapsed >= 1.0f) {
            tick_rate_hz_.store(tick_count / rate_elapsed, std::memory_order_relaxed);
            tick_count = 0;
            rate_start = tick_end;
        }

        RenderState* buf = render_buffer.get_producer_buf();
        *buf = state;
        render_buffer.present();

        // Sleep the remainder of the 16ms frame target
        auto frame_end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - now).count();
        if (elapsed < 16)
            std::this_thread::sleep_for(std::chrono::milliseconds(16 - elapsed));
    }
}
