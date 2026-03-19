/**
 * @file IRenderer.h
 * @brief Abstract renderer interface for backend-agnostic drawing.
 */
#pragma once

#include <cstdint>

class RenderState;

/**
 * @brief Backend-agnostic renderer interface.
 *
 * Implementations (GL, Vulkan, software, etc.) draw a RenderState to an
 * offscreen target and return an opaque texture handle. Each backend
 * provides its own concrete subclass.
 */
class IRenderer {
  public:
    /** @brief Virtual destructor. */
    virtual ~IRenderer() = default;

    /**
     * @brief Draw the given render state and return an opaque texture handle.
     *
     * Renders all layer groups in @p state to an offscreen framebuffer and
     * returns a handle suitable for compositing (e.g. a GLuint cast to uint32_t).
     *
     * @param state Pointer to the RenderState describing what to draw. May be null.
     * @return Opaque GPU texture handle representing the rendered frame.
     */
    virtual uint32_t draw(const RenderState* state) = 0;

    /**
     * @brief Bind the default framebuffer (the screen).
     *
     * Restores rendering output to the window's default framebuffer so that
     * subsequent draw calls target the display.
     */
    virtual void bind_default_framebuffer() = 0;

    /**
     * @brief Clear the currently bound framebuffer.
     */
    virtual void clear() = 0;
};
