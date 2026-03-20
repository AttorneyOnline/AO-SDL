#pragma once

#include "ui/IUIRenderer.h"
#include "ui/widgets/ChatWidget.h"
#include "ui/widgets/CharSelectWidget.h"
#include "ui/widgets/CourtroomWidget.h"
#include "ui/widgets/ServerListWidget.h"

#include <memory>
#include <string>

class ImGuiUIRenderer : public IUIRenderer {
  public:
    void begin_frame() override;
    void render_screen(Screen& screen, RenderManager& render) override;
    void end_frame() override;

  private:
    void bind_screen(Screen& screen, RenderManager& render);

    ChatWidget chat_;

    std::string active_screen_id_;
    std::unique_ptr<ServerListWidget> server_list_;
    std::unique_ptr<CharSelectWidget> char_select_;
    std::unique_ptr<CourtroomWidget> courtroom_;
};
