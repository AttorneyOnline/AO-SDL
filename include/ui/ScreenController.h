/**
 * @file ScreenController.h
 * @brief Interface for screen stack navigation (push_screen, pop_screen).
 */
#pragma once

#include <memory>

class Screen;

/**
 * @brief Minimal interface exposed to screens for driving stack transitions.
 *
 * Decouples individual Screen implementations from UIManager, allowing them
 * to push or pop screens without a direct dependency on the manager.
 */
class ScreenController {
  public:
    /**
     * @brief Push a new screen onto the stack, making it the active screen.
     *
     * The current active screen's exit() is called before the new screen's
     * enter().
     *
     * @param screen The screen to push. Ownership is transferred to the stack.
     */
    virtual void push_screen(std::unique_ptr<Screen> screen) = 0;

    /**
     * @brief Pop the active screen off the stack.
     *
     * The popped screen's exit() is called. If a screen remains underneath,
     * its enter() is called to reactivate it.
     */
    virtual void pop_screen() = 0;

    /** @brief Virtual destructor. */
    virtual ~ScreenController() = default;
};
