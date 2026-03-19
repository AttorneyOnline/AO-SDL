#pragma once

#include "ui/Screen.h"
#include "ui/ChatWidget.h"
#include "render/Texture.h"

#include <optional>
#include <string>
#include <vector>

class CharSelectScreen : public Screen {
  public:
    void enter(ScreenController& controller) override;
    void exit() override;
    void handle_events() override;
    void render(RenderManager& render) override;

  private:
    void load_icons();

    struct CharEntry {
        std::string folder;
        std::optional<Texture2D> icon;
        bool taken = false;
    };

    ScreenController* controller = nullptr;
    std::vector<CharEntry> chars;
    int selected = -1;

    ChatWidget chat;
};
