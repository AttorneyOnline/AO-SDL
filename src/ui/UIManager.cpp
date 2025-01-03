#include "UIManager.h"

#include <imgui.h>

#include "event/EventManager.h"
#include "utils/Log.h"

#include <format>

UIManager::UIManager() : current_view(SERVER_LIST) {
}

void UIManager::handle_events() {
    EventChannel<UIEvent>* event_channel = EventManager::get_instance().get_ui_channel();

    while (auto optev = event_channel->get_event()) {
        UIEvent ev = *optev;
        Log::log_print(DEBUG, "Received UI event: %s", ev.to_string().c_str());

        switch (ev.get_type()) {
        case UIEventType::CHAR_LOADING_DONE:
            current_view = CHAR_SELECT;
            break;
        }
    }

    // todo: put this in the correct place!!!!!
    EventChannel<ChatEvent>* chat_event_channel = EventManager::get_instance().get_chat_channel();
    auto optev = chat_event_channel->get_event();
    if (optev) {
        ChatEvent ev = *optev;
        chat_buffer = std::format("{}\n{}", chat_buffer, ev.to_string());
    }
}

// todo: this should probably instead exist in some sort of "UIRenderer" class, and UIManager shouldn't be exposed to
// the details of RenderManager shouldn't been too difficult to refactor, though. just do it soon before this class
// becomes horrible.
void UIManager::render_current_view(RenderManager& render) {
    // Right now the plan is to only have these three UI views, but in the future, it will probably make sense to
    // think up a more generalized solution to this problem.
    if (current_view == COURTROOM) {
        // Render viewport to texture
        // This internally calls glBindFramebuffer, but we should change that, and manage the render output texture
        // outside of the scope of the RenderManager class. We should probably just get rid of RenderManager and
        // directly use Renderer.cpp in the future
        uint32_t render_texture = render.render_frame();

        // Start rendering to our viewport framebuffer
        render.bind_framebuffer(0);

        // Show viewport
        ImGui::Begin("Courtroom Viewport Yay");
        ImGui::Image(render_texture, ImGui::GetContentRegionAvail(), {0, 1}, {1, 0});
        ImGui::End();
    }
    else if (current_view == CHAR_SELECT) {
        ImGui::Begin("Character Selector");
        ImGui::Text("We finished getting all of the char data from the server\nso now we have notified the UI\nand "
                    "changed to the CHAR_SELECT view");
        ImGui::End();

        ImGui::Begin("Chat");
        ImGui::Text(chat_buffer.c_str());
        ImGui::End();
    }
    else if (current_view == SERVER_LIST) {
        render.bind_framebuffer(0);

        ImGui::Begin("Servers...");
        ImGui::Text("hello, world! (changeme)");
        ImGui::End();
    }

    render.clear_framebuffer();
    ImGui::Render();
}

UIView UIManager::get_current_view() {
    return current_view;
}