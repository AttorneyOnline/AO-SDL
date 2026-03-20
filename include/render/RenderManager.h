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
    RenderManager(StateBuffer& buf, std::unique_ptr<IRenderer> renderer);

    /// Draw the current game state to the offscreen render target.
    void render_frame();

    /// Bind the default framebuffer and clear it.
    void begin_frame();

    /// Access the underlying renderer.
    IRenderer& get_renderer() {
        return *renderer;
    }

  private:
    StateBuffer& state_buf;
    std::unique_ptr<IRenderer> renderer;
};
