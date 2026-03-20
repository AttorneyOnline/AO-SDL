#pragma once

#include "ui/controllers/IScreenController.h"
#include "ui/widgets/CharSelectWidget.h"
#include "ui/widgets/ChatWidget.h"

#include <memory>

class CharSelectScreen;
class RenderManager;

class CharSelectController : public IScreenController {
  public:
    CharSelectController(CharSelectScreen& screen, RenderManager& render);
    void render() override;
    IUIRenderer::NavAction nav_action() override;

  private:
    std::unique_ptr<CharSelectWidget> char_select_;
    ChatWidget chat_;
    IUIRenderer::NavAction nav_action_ = IUIRenderer::NavAction::NONE;
};
