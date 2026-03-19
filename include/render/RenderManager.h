/**
 * @file RenderManager.h
 * @brief Owns an IRenderer and a StateBuffer; called from the render loop.
 */
#pragma once

#include "IRenderer.h"
#include "StateBuffer.h"

#include <cstdint>
#include <memory>

/**
 * @brief Manages per-frame rendering by combining a StateBuffer with an IRenderer.
 *
 * RenderManager is intended to be driven from the render thread. It pulls the
 * latest committed state from the StateBuffer and delegates drawing to the
 * owned IRenderer.
 */
class RenderManager {
  public:
    /**
     * @brief Construct a RenderManager.
     *
     * @param buf Reference to the triple-buffered StateBuffer shared with the game thread.
     * @param renderer Unique pointer to the renderer backend. Ownership is transferred
     *                 to RenderManager; the caller must not use the pointer after this call.
     */
    RenderManager(StateBuffer& buf, std::unique_ptr<IRenderer> renderer);

    /**
     * @brief Render the current frame.
     *
     * Fetches the latest consumer state from the StateBuffer, passes it to the
     * owned IRenderer, and returns the resulting opaque texture handle.
     *
     * @return Opaque GPU texture handle for the rendered frame.
     */
    uint32_t render_frame();

    /**
     * @brief Prepare for a new frame.
     *
     * Binds the default framebuffer and clears it, readying the display for
     * the next composited output.
     */
    void begin_frame();

  private:
    StateBuffer& state_buf;                ///< Shared triple-buffer for game-to-render communication.
    std::unique_ptr<IRenderer> renderer;   ///< Owned renderer backend.
};
