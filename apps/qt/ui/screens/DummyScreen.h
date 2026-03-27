#pragma once

#include "ui/Screen.h"

/**
 * @brief Placeholder screen pushed by the ServerList "Push Dummy Screen" button.
 *
 * Exists solely to test navigation: ServerList → Dummy → back.
 * Has no event handling and no game logic.  Remove in Phase 3 once real
 * screens (CharSelect, Courtroom) are wired.
 */
class DummyScreen final : public Screen {
  public:
    inline static const std::string ID = "dummy";

    void enter(ScreenController& ctrl) override { m_ctrl = &ctrl; }
    void exit()                         override { m_ctrl = nullptr; }
    void handle_events()                override {}

    const std::string& screen_id() const override { return ID; }

    ScreenController* ctrl() const { return m_ctrl; }

  private:
    ScreenController* m_ctrl = nullptr;
};
