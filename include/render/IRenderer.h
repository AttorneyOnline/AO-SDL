/**
 * @file IRenderer.h
 * @brief Abstract renderer interface for backend-agnostic drawing.
 */
#pragma once

#include <cstdint>
#include <memory>

class ImageAsset;
class RenderState;
class Layer;

/**
 * @brief Backend-agnostic renderer interface.
 *
 * Implementations (GL, Metal, Vulkan, software, etc.) draw a RenderState to an
 * offscreen target and return an opaque texture handle. Each backend
 * provides its own concrete subclass.
 */
class IRenderer {
  public:
    /** @brief Virtual destructor. */
    virtual ~IRenderer() = default;

    /**
     * @brief Draw the given render state to the offscreen target.
     *
     * @param state Pointer to the RenderState describing what to draw. May be null.
     */
    virtual void draw(const RenderState* state) = 0;

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

    /**
     * @brief Get the offscreen render texture as an opaque ID suitable for
     *        toolkit compositing (e.g. ImGui::Image).
     *
     * On GL this is a GLuint cast to uintptr_t.
     * On Metal this is an id<MTLTexture> bridged to uintptr_t.
     */
    virtual uintptr_t get_render_texture_id() const = 0;

    /**
     * @brief Get a texture suitable for on-screen display at the given size.
     *
     * Returns a texture that has been upscaled from the offscreen render target
     * using nearest-neighbor filtering. Backends where the toolkit already
     * samples with nearest filtering (e.g. GL) may simply return the render
     * texture unchanged.
     *
     * @param display_w Desired display width in pixels.
     * @param display_h Desired display height in pixels.
     */
    virtual uintptr_t get_display_texture_id(int display_w, int display_h) {
        (void)display_w;
        (void)display_h;
        return get_render_texture_id();
    }

    /**
     * @brief Whether texture V=0 is at the bottom (true for GL, false for Metal).
     *
     * Toolkits that display the render texture need to know the UV convention
     * so they can flip if necessary.
     */
    virtual bool uv_flipped() const = 0;

    /**
     * @brief Get an opaque pointer to the GPU device handle, or nullptr.
     *
     * Used during backend initialisation (e.g. to set up ImGui for Metal).
     * Returns nullptr on backends that don't need it (GL).
     */
    virtual void* get_device_ptr() const {
        return nullptr;
    }

    /**
     * @brief Get an opaque pointer to the command queue, or nullptr.
     */
    virtual void* get_command_queue_ptr() const {
        return nullptr;
    }

    /**
     * @brief Resize the offscreen render target.
     *
     * Recreates the framebuffer and depth buffer at the new dimensions.
     * Called from the render thread when the internal resolution scale changes.
     */
    virtual void resize(int width, int height) = 0;

    /// Toggle wireframe rendering mode (for debugging mesh geometry).
    virtual void set_wireframe(bool enabled) { (void)enabled; }

    /// Get the GPU texture handle for an ImageAsset (for ImGui preview).
    /// Returns 0 if the asset has not been uploaded. Does not trigger upload.
    virtual uintptr_t get_texture_id(const std::shared_ptr<ImageAsset>& asset) { (void)asset; return 0; }

    virtual const char* backend_name() const = 0;

    /// Number of draw calls in the last frame.
    int last_draw_calls() const { return draw_calls_; }

  protected:
    int draw_calls_ = 0;
};
