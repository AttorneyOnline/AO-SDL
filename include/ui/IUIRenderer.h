/**
 * @file IUIRenderer.h
 * @brief Abstract interface for UI rendering backends (ImGui, Qt, etc.).
 */
#pragma once

class Screen;
class RenderManager;

/**
 * @brief Abstract interface for UI rendering backends.
 *
 * Implementations translate Screen state into visible UI using their
 * respective toolkit (ImGui, Qt/QML, etc.). Each frame follows the sequence:
 * begin_frame(), render_screen(), end_frame().
 */
class IUIRenderer {
  public:
    /** @brief Virtual destructor. */
    virtual ~IUIRenderer() = default;

    /**
     * @brief Set up any per-frame state the backend needs before rendering.
     *
     * Called once at the start of each frame before render_screen().
     */
    virtual void begin_frame() = 0;

    /**
     * @brief Render the active screen.
     *
     * The implementation should dispatch on screen.screen_id() and
     * dynamic_cast to the concrete screen type to access its state.
     *
     * @param screen The active Screen whose state should be rendered.
     * @param render The RenderManager providing the scene texture for compositing.
     */
    virtual void render_screen(Screen& screen, RenderManager& render) = 0;

    /**
     * @brief Finalize and present the frame.
     *
     * Called once at the end of each frame after render_screen().
     */
    virtual void end_frame() = 0;

    enum class NavAction { NONE, POP_SCREEN, POP_TO_ROOT };

    virtual NavAction pending_nav_action() {
        return NavAction::NONE;
    }
};
