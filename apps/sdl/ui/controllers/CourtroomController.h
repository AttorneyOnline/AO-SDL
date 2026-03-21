#pragma once

#include "ui/controllers/IScreenController.h"
#include "ui/widgets/ChatWidget.h"
#include "ui/widgets/CourtroomWidget.h"
#include "ui/widgets/DebugOverlayWidget.h"
#include "ui/widgets/ICLogWidget.h"
#include "ui/widgets/EmoteSelectorWidget.h"
#include "ui/widgets/ICChatWidget.h"
#include "ui/widgets/ICMessageState.h"
#include "ui/widgets/InterjectionWidget.h"
#include "ui/widgets/MessageOptionsWidget.h"
#include "ui/widgets/MusicAreaWidget.h"
#include "ui/widgets/SideSelectWidget.h"

#include <memory>

class CourtroomScreen;
class RenderManager;

class CourtroomController : public IScreenController {
  public:
    CourtroomController(CourtroomScreen& screen, RenderManager& render);
    void render() override;
    IUIRenderer::NavAction nav_action() override;

  private:
    void update_debug_stats();

    ChatWidget chat_;
    ICMessageState ic_state_;
    IUIRenderer::NavAction nav_action_ = IUIRenderer::NavAction::NONE;

    std::unique_ptr<CourtroomWidget> courtroom_;
    std::unique_ptr<ICChatWidget> ic_chat_;
    std::unique_ptr<EmoteSelectorWidget> emote_selector_;
    std::unique_ptr<InterjectionWidget> interjection_;
    std::unique_ptr<SideSelectWidget> side_select_;
    std::unique_ptr<MessageOptionsWidget> message_options_;
    ICLogWidget ic_log_;
    std::unique_ptr<MusicAreaWidget> music_area_;
    DebugOverlayWidget debug_;
    RenderManager* render_ = nullptr;
    bool debug_open_ = false; // toggled by /debug in OOC chat
};
