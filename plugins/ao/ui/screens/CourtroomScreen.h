#pragma once

#include "ui/Screen.h"

class CourtroomScreen : public Screen {
  public:
    static inline const std::string ID = "courtroom";

    void enter(ScreenController& controller) override;
    void exit() override;
    void handle_events() override;

    const std::string& screen_id() const override {
        return ID;
    }
};
