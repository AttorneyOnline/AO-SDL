#pragma once

#include "asset/ImageAsset.h"
#include "game/ICharacterSheet.h"
#include "ui/Screen.h"

#include <future>
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

    /// Update the character without destroying courtroom state.
    /// Reloads character sheet and emote icons asynchronously.
    void change_character(const std::string& character_name, int char_id);

    /// True while character data is still being loaded asynchronously.
    bool is_loading() const {
        return loading_;
    }

    /// Character sheet loaded from the protocol plugin. May be null.
    const std::shared_ptr<ICharacterSheet>& get_character_sheet() const {
        return char_sheet_;
    }

    /// Emote button icons loaded from the protocol plugin.
    const std::vector<std::shared_ptr<ImageAsset>>& get_emote_icons() const {
        return emote_icons_;
    }

    /// Incremented each time loaded data changes. Controller watches this
    /// to know when to reinitialize emote icons, etc.
    int load_generation() const {
        return load_generation_;
    }

  private:
    void load_character_data();

    std::string character_name_;
    int char_id_;
    bool loading_ = true;
    int load_generation_ = 0;
    std::shared_ptr<ICharacterSheet> char_sheet_;
    std::vector<std::shared_ptr<ImageAsset>> emote_icons_;
    std::future<void> load_future_;
};
