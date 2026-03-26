/**
 * @file GameThread.h
 * @brief Spawns a dedicated thread that calls IScenePresenter::tick() at ~10 Hz.
 */
#pragma once

#include "IScenePresenter.h"
#include "render/StateBuffer.h"

#include <atomic>
#include <thread>

/**
 * @brief Drives an IScenePresenter on a background thread at approximately 10 Hz.
 *
 * On construction, a std::jthread is spawned running game_loop(). Each iteration
 * calls IScenePresenter::tick() and publishes the resulting RenderState into the
 * shared StateBuffer for the render thread to consume.
 *
 * Call stop() to signal the thread to exit and join it, or let the destructor
 * handle it automatically.
 */
class GameThread {
  public:
    /**
     * @brief Construct a GameThread and immediately spawn the game loop thread.
     * @param render_buffer Shared buffer where each tick's RenderState is published.
     *                      Must outlive this GameThread.
     * @param presenter The scene presenter whose tick() method will be called.
     *                  Must outlive this GameThread.
     */
    GameThread(StateBuffer& render_buffer, IScenePresenter& presenter);

    /**
     * @brief Signal the game loop to stop and join the thread.
     */
    void stop();

    /// Last tick duration in microseconds (thread-safe read).
    int last_tick_us() const {
        return last_tick_us_.load(std::memory_order_relaxed);
    }
    float tick_rate_hz() const {
        return tick_rate_hz_.load(std::memory_order_relaxed);
    }

  private:
    void game_loop(std::stop_token st);

    std::atomic<int> last_tick_us_;   /**< Last tick() duration in microseconds. */
    std::atomic<float> tick_rate_hz_; /**< Measured tick frequency. */
    StateBuffer& render_buffer;       /**< Shared render state output buffer. */
    IScenePresenter& presenter;       /**< Scene presenter driven each tick. */
    std::jthread tick_thread;         /**< The background game thread. */
};
