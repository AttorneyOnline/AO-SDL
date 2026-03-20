#pragma once

#include "ui/Screen.h"

#include <string>

class CourtroomScreen : public Screen {
  public:
    static inline const std::string ID = "courtroom";

    CourtroomScreen(std::string character_name, int char_id);

    void enter(ScreenController& controller) override;
    void exit() override;
    void handle_events() override;

    const std::string& screen_id() const override {
        return ID;
    }

    const std::string& get_character_name() const {
        return character_name_;
    }
    int get_char_id() const {
        return char_id_;
    }

  private:
    std::string character_name_;
    int char_id_;
};
