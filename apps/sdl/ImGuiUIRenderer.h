#pragma once

#include "ui/IUIRenderer.h"

// ImGui implementation of the UI rendering backend.
class ImGuiUIRenderer : public IUIRenderer {
  public:
    void begin_frame() override;
    void render_screen(Screen& screen, RenderManager& render) override;
    void end_frame() override;

  private:
    void render_server_list(class ServerListScreen& screen, RenderManager& render);
    void render_char_select(class CharSelectScreen& screen, RenderManager& render);
    void render_courtroom(class CourtroomScreen& screen, RenderManager& render);

    void render_chat(class ChatWidget& chat);
};
