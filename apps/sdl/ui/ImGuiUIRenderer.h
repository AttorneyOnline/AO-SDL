#pragma once

#include "ui/IUIRenderer.h"
#include "ui/controllers/IScreenController.h"
#include "ui/widgets/DisconnectModalWidget.h"

#include <memory>
#include <string>

class ImGuiUIRenderer : public IUIRenderer {
  public:
    void begin_frame() override;
    void render_screen(Screen& screen, RenderManager& render) override;
    void end_frame() override;

    NavAction pending_nav_action() override;

  private:
    std::unique_ptr<IScreenController> create_controller(Screen& screen, RenderManager& render);

    DisconnectModalWidget disconnect_modal_;
    std::string active_screen_id_;
    std::unique_ptr<IScreenController> controller_;
};
