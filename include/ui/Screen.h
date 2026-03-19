/**
 * @file Screen.h
 * @brief Abstract base class for UI screens (enter, exit, handle_events, screen_id).
 */
#pragma once

#include "ScreenController.h"

#include <string>

/**
 * @brief Abstract base class representing a single UI screen in the screen stack.
 *
 * Subclasses implement the lifecycle methods (enter, exit) and per-frame event
 * handling. Each screen provides a unique string identifier for backend dispatch.
 */
class Screen {
  public:
    /** @brief Virtual destructor. */
    virtual ~Screen() = default;

    /**
     * @brief Called when this screen becomes the top of the stack.
     *
     * The provided controller is valid for the lifetime of this screen's active
     * period. Store it if you need to push or pop screens from handle_events().
     *
     * @param controller Interface for driving screen stack transitions.
     */
    virtual void enter(ScreenController& controller) = 0;

    /**
     * @brief Called when this screen is popped or covered by a pushed screen.
     */
    virtual void exit() = 0;

    /**
     * @brief Process pending events for this frame.
     *
     * Called once per frame by UIManager for the active (topmost) screen.
     */
    virtual void handle_events() = 0;

    /**
     * @brief Get the unique screen identifier.
     *
     * Used by the UI rendering backend to dispatch to the correct rendering
     * logic for this screen type.
     *
     * @return A const reference to the screen's unique string ID.
     */
    virtual const std::string& screen_id() const = 0;
};
