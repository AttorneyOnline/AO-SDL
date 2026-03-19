#include "CourtroomScreen.h"

#include <imgui.h>

void CourtroomScreen::enter(ScreenController&) {}

void CourtroomScreen::exit() {}

void CourtroomScreen::handle_events() {
    chat.handle_events();
}

void CourtroomScreen::render(RenderManager& render) {
    uint32_t render_texture = render.render_frame();

    render.begin_frame();

    ImGui::Begin("Courtroom");
    ImGui::Image(render_texture, ImGui::GetContentRegionAvail(), {0, 1}, {1, 0});
    ImGui::End();

    chat.render();

    ImGui::Render();
}
