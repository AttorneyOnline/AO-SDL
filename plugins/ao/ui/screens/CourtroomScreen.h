#pragma once

#include "asset/ImageAsset.h"
#include "game/ICharacterSheet.h"
#include "ui/Screen.h"

#include <memory>
#include <string>
#include <vector>

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

    /// Character sheet loaded from the protocol plugin. May be null.
    const ICharacterSheet* get_character_sheet() const {
        return char_sheet_.get();
    }

    /// Emote button icons loaded from the protocol plugin.
    const std::vector<std::shared_ptr<ImageAsset>>& get_emote_icons() const {
        return emote_icons_;
    }

  private:
    std::string character_name_;
    int char_id_;
    std::shared_ptr<ICharacterSheet> char_sheet_;
    std::vector<std::shared_ptr<ImageAsset>> emote_icons_;
};
