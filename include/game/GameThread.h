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
 * On construction, a std::thread is spawned running game_loop(). Each iteration
 * calls IScenePresenter::tick() and publishes the resulting RenderState into the
 * shared StateBuffer for the render thread to consume.
 *
 * Call stop() to signal the thread to exit and join it.
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
     *
     * Sets the running flag to false and blocks until the thread exits.
     */
    void stop();

  private:
    /**
     * @brief Main loop executed on the game thread.
     *
     * Repeatedly calls presenter.tick() at ~10 Hz and writes the result
     * into the render buffer until running becomes false.
     */
    void game_loop();

    std::atomic<bool> running;      /**< Flag to signal the loop to exit. */
    StateBuffer& render_buffer;     /**< Shared render state output buffer. */
    IScenePresenter& presenter;     /**< Scene presenter driven each tick. */
    std::thread tick_thread;        /**< The background game thread. */
};
