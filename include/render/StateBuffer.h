/**
 * @file StateBuffer.h
 * @brief Triple-buffer for thread-safe game-to-render state communication.
 */
#pragma once

#include "RenderState.h"

#include <atomic>
#include <mutex>

/**
 * @brief Triple-buffer that allows the game thread to publish RenderState
 *        without blocking the render thread.
 *
 * Internally manages three RenderState slots (preparing, ready, presenting)
 * and uses a mutex to safely swap the ready and presenting pointers.
 *
 * **Threading contract:**
 * - Game thread calls get_producer_buf() and present().
 * - Render thread calls update() and get_consumer_buf().
 *
 * @note Uses an internal std::mutex for the swap operation and an atomic flag
 *       to avoid unnecessary locking when no new data is available.
 */
class StateBuffer {
  public:
    /** @brief Default-construct with default-initialized RenderState buffers. */
    StateBuffer();

    /**
     * @brief Construct with an initial RenderState copied into each buffer slot.
     * @param initial_buf The initial state to populate all three buffers with.
     */
    StateBuffer(RenderState initial_buf);

    /**
     * @brief Get the buffer the game thread writes into.
     *
     * The returned pointer remains valid until the next call to present().
     * Only the game (producer) thread should call this.
     *
     * @return Pointer to the writable producer RenderState.
     */
    RenderState* get_producer_buf();

    /**
     * @brief Get the buffer the render thread reads from.
     *
     * The returned pointer remains valid until the next call to update().
     * Only the render (consumer) thread should call this.
     *
     * @return Pointer to the read-only consumer RenderState.
     */
    const RenderState* get_consumer_buf();

    /**
     * @brief Publish the current producer buffer, making it available to the render thread.
     *
     * Swaps the preparing and ready pointers under a mutex lock and sets the
     * stale flag so the render thread knows new data is available.
     *
     * **Called by the game thread.**
     */
    void present();

    /**
     * @brief Consume the latest published state, if any.
     *
     * If the stale flag is set, swaps the ready and presenting pointers under
     * a mutex lock. Otherwise this is a no-op.
     *
     * **Called by the render thread.**
     */
    void update();

  private:
    RenderState* presenting;   ///< Buffer currently being read by the render thread.
    RenderState* ready;        ///< Buffer holding the most recently published state.
    RenderState* preparing;    ///< Buffer currently being written by the game thread.

    std::atomic<bool> stale;   ///< True when the game thread has published new data.
    std::mutex swap_mutex;     ///< Guards swaps between ready and presenting/preparing.
};
