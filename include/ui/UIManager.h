/**
 * @file UIManager.h
 * @brief Owns the screen stack and dispatches per-frame events to the active screen.
 */
#pragma once

#include "Screen.h"
#include "ScreenController.h"

#include <memory>
#include <vector>

/**
 * @brief Owns and manages the screen stack, implementing ScreenController.
 *
 * UIManager maintains a stack of Screen instances and forwards per-frame
 * handle_events() calls to the topmost (active) screen.
 */
class UIManager : public ScreenController {
  public:
    /** @brief Construct an empty UIManager with no screens on the stack. */
    UIManager();

    /**
     * @brief Push a new screen onto the stack.
     *
     * Calls exit() on the current active screen (if any), then enter() on
     * the new screen.
     *
     * @param screen The screen to push. Ownership is transferred to the manager.
     */
    void push_screen(std::unique_ptr<Screen> screen) override;

    /**
     * @brief Pop the active screen off the stack.
     *
     * Calls exit() on the active screen and destroys it. If a screen remains
     * underneath, its enter() is called to reactivate it.
     */
    void pop_screen() override;

    /**
     * @brief Dispatch event handling to the active screen.
     *
     * Called once per frame. Forwards to the active screen's handle_events()
     * if the stack is non-empty.
     */
    void handle_events();

    /**
     * @brief Get the active (topmost) screen.
     * @return Pointer to the active screen, or nullptr if the stack is empty.
     */
    Screen* active_screen() const;

  private:
    std::vector<std::unique_ptr<Screen>> stack; ///< Screen stack; back() is the active screen.
};
