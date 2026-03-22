/**
 * @file CharSelectScreen.h
 * @brief Character selection screen state.
 */
#pragma once

#include "render/Texture.h"
#include "ui/Screen.h"

#include <optional>
#include <string>
#include <vector>

/**
 * @brief Screen for selecting a character from the available roster.
 *
 * Displays character icons and allows the user to pick one. Includes an
 * embedded ChatWidget for in-lobby chat.
 *
 * @note Consumes CharacterListEvent (to populate the roster),
 *       CharsCheckEvent (to mark taken characters), and UIEvent.
 *       Publishes CharSelectRequestEvent when the user confirms a selection
 *       via select_character().
 */
class CharSelectScreen : public Screen {
  public:
    /** @brief Unique screen identifier for backend dispatch. */
    static inline const std::string ID = "char_select";

    /**
     * @brief A single entry in the character roster.
     */
    struct CharEntry {
        std::string folder;            ///< Character asset folder name.
        std::optional<Texture2D> icon; ///< Character icon texture (absent if not yet loaded).
        bool taken = false;            ///< True if another player has claimed this character.
    };

    /**
     * @brief Called when this screen becomes active.
     *
     * Stores the controller reference and begins loading character data.
     *
     * @param controller Interface for screen stack navigation.
     */
    void enter(ScreenController& controller) override;

    /** @brief Called when this screen is deactivated or popped. */
    void exit() override;

    /**
     * @brief Process pending events for this frame.
     *
     * Consumes CharacterListEvent, CharsCheckEvent, and UIEvent instances.
     * Delegates to ChatWidget::handle_events() for chat processing.
     */
    void handle_events() override;

    /**
     * @brief Get the screen identifier.
     * @return Reference to the static ID string "char_select".
     */
    const std::string& screen_id() const override {
        return ID;
    }

    /**
     * @brief Get the character roster.
     * @return Const reference to the vector of CharEntry items.
     */
    const std::vector<CharEntry>& get_chars() const {
        return chars;
    }

    /**
     * @brief Get the index of the currently selected character.
     * @return Selected character index, or -1 if none is selected.
     */
    int get_selected() const {
        return selected;
    }

    /**
     * @brief Select a character and request it from the server.
     *
     * Records the selection and publishes a CharSelectRequestEvent.
     *
     * @param index Zero-based index into the character roster.
     */
    void select_character(int index);

  private:
    void retry_icons();

    ScreenController* controller = nullptr; ///< Stored controller for stack navigation.
    std::vector<CharEntry> chars;           ///< Character roster entries.
    int selected = -1;                      ///< Currently selected character index (-1 = none).
    int prefetch_cursor_ = 0;               ///< Next character index to prefetch.
    int retry_cursor_ = 0;                  ///< Next character index to retry after transient failure.
};
