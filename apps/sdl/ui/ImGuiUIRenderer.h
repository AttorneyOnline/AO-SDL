#pragma once

#include "ui/IUIRenderer.h"
#include "ui/widgets/CharSelectWidget.h"
#include "ui/widgets/ChatWidget.h"
#include "ui/widgets/CourtroomWidget.h"
#include "ui/widgets/DisconnectModalWidget.h"
#include "ui/widgets/EmoteSelectorWidget.h"
#include "ui/widgets/ICChatWidget.h"
#include "ui/widgets/ICMessageState.h"
#include "ui/widgets/InterjectionWidget.h"
#include "ui/widgets/MessageOptionsWidget.h"
#include "ui/widgets/ServerListWidget.h"
#include "ui/widgets/SideSelectWidget.h"

#include <memory>
#include <string>

class ImGuiUIRenderer : public IUIRenderer {
  public:
    void begin_frame() override;
    void render_screen(Screen& screen, RenderManager& render) override;
    void end_frame() override;

    NavAction pending_nav_action() override;

  private:
    void bind_screen(Screen& screen, RenderManager& render);

    ChatWidget chat_;
    ICMessageState ic_state_;
    DisconnectModalWidget disconnect_modal_;
    NavAction nav_action_ = NavAction::NONE;

    std::string active_screen_id_;
    std::unique_ptr<ServerListWidget> server_list_;
    std::unique_ptr<CharSelectWidget> char_select_;
    std::unique_ptr<CourtroomWidget> courtroom_;

    // IC widgets — created when entering courtroom
    std::unique_ptr<ICChatWidget> ic_chat_;
    std::unique_ptr<EmoteSelectorWidget> emote_selector_;
    std::unique_ptr<InterjectionWidget> interjection_;
    std::unique_ptr<SideSelectWidget> side_select_;
    std::unique_ptr<MessageOptionsWidget> message_options_;
};
