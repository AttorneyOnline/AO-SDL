/**
 * @file CourtroomScreen.h
 * @brief Courtroom screen state (stub).
 */
#pragma once

#include "ui/ChatWidget.h"
#include "ui/Screen.h"

/**
 * @brief Courtroom screen where gameplay takes place.
 *
 * This is currently a stub. It provides an embedded ChatWidget for in-court
 * messaging and implements the Screen lifecycle interface.
 */
class CourtroomScreen : public Screen {
  public:
    /** @brief Unique screen identifier for backend dispatch. */
    static inline const std::string ID = "courtroom";

    /**
     * @brief Called when this screen becomes active.
     * @param controller Interface for screen stack navigation.
     */
    void enter(ScreenController& controller) override;

    /** @brief Called when this screen is deactivated or popped. */
    void exit() override;

    /**
     * @brief Process pending events for this frame.
     *
     * Delegates to ChatWidget::handle_events() for chat processing.
     */
    void handle_events() override;

    /**
     * @brief Get the screen identifier.
     * @return Reference to the static ID string "courtroom".
     */
    const std::string& screen_id() const override {
        return ID;
    }

    /**
     * @brief Get the embedded chat widget (mutable).
     * @return Mutable reference to the ChatWidget.
     */
    ChatWidget& get_chat() {
        return chat;
    }

    /**
     * @brief Get the embedded chat widget (const).
     * @return Const reference to the ChatWidget.
     */
    const ChatWidget& get_chat() const {
        return chat;
    }

  private:
    ChatWidget chat; ///< Embedded chat widget for courtroom messaging.
};
