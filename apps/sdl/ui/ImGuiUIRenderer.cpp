#include "ImGuiUIRenderer.h"

#include "ao/asset/AOAssetLibrary.h"
#include "ao/ui/screens/CharSelectScreen.h"
#include "ao/ui/screens/CourtroomScreen.h"
#include "ao/ui/screens/ServerListScreen.h"
#include "asset/MediaManager.h"
#include "ui/Screen.h"

#include <imgui.h>

void ImGuiUIRenderer::begin_frame() {
    ImGui::NewFrame();
}

IUIRenderer::NavAction ImGuiUIRenderer::pending_nav_action() {
    if (disconnect_modal_.should_return_to_server_list()) {
        disconnect_modal_.clear_flag();
        active_screen_id_.clear();
        return NavAction::POP_TO_ROOT;
    }
    NavAction action = nav_action_;
    nav_action_ = NavAction::NONE;
    if (action != NavAction::NONE)
        active_screen_id_.clear();
    return action;
}

void ImGuiUIRenderer::render_screen(Screen& screen, RenderManager& render) {
    bind_screen(screen, render);

    disconnect_modal_.handle_events();
    disconnect_modal_.render();

    const auto& id = screen.screen_id();

    if (id == ServerListScreen::ID) {
        server_list_->render();
    }
    else if (id == CharSelectScreen::ID) {
        char_select_->render();
        chat_.handle_events();
        chat_.render();

        ImGui::Begin("Connection");
        if (ImGui::Button("Disconnect"))
            nav_action_ = NavAction::POP_TO_ROOT;
        ImGui::End();
    }
    else if (id == CourtroomScreen::ID) {
        courtroom_->render();
        chat_.handle_events();
        chat_.render();
        emote_selector_->render();
        interjection_->render();
        side_select_->render();
        message_options_->render();
        ic_chat_->render();

        ImGui::Begin("Connection");
        if (ImGui::Button("Change Character"))
            nav_action_ = NavAction::POP_SCREEN;
        ImGui::SameLine();
        if (ImGui::Button("Disconnect"))
            nav_action_ = NavAction::POP_TO_ROOT;
        ImGui::End();
    }
}

void ImGuiUIRenderer::end_frame() {
    ImGui::Render();
}

static int side_to_index(const std::string& side) {
    static constexpr const char* SIDES[] = {"def", "pro", "wit", "jud", "jur", "sea", "hlp"};
    for (int i = 0; i < 7; i++) {
        if (side == SIDES[i])
            return i;
    }
    return 2; // default: wit
}

void ImGuiUIRenderer::bind_screen(Screen& screen, RenderManager& render) {
    const auto& id = screen.screen_id();
    if (id == active_screen_id_)
        return;

    active_screen_id_ = id;

    if (id == ServerListScreen::ID) {
        server_list_ = std::make_unique<ServerListWidget>(static_cast<ServerListScreen&>(screen), render);
    }
    else if (id == CharSelectScreen::ID) {
        char_select_ = std::make_unique<CharSelectWidget>(static_cast<CharSelectScreen&>(screen), render);
    }
    else if (id == CourtroomScreen::ID) {
        auto& court = static_cast<CourtroomScreen&>(screen);

        // Populate IC state from char.ini
        ic_state_ = {};
        ic_state_.character = court.get_character_name();
        ic_state_.char_id = court.get_char_id();

        AOAssetLibrary ao_assets(MediaManager::instance().assets());
        ic_state_.char_sheet = ao_assets.character_sheet(ic_state_.character);

        // Set default side and showname from char.ini
        if (ic_state_.char_sheet) {
            ic_state_.side_index = side_to_index(ic_state_.char_sheet->side());
            std::strncpy(ic_state_.showname, ic_state_.char_sheet->showname().c_str(),
                         sizeof(ic_state_.showname) - 1);
        }

        // Load emote icons
        int emote_count = ic_state_.char_sheet ? ic_state_.char_sheet->emote_count() : 0;
        for (int i = 0; i < emote_count; i++) {
            EmoteIcon icon;
            icon.comment = ic_state_.char_sheet->emote(i).comment;

            auto img = ao_assets.emote_icon(ic_state_.character, i);
            if (img && img->frame_count() > 0) {
                const ImageFrame& frame = img->frame(0);
                icon.icon.emplace(frame.width, frame.height, frame.pixels.data(), 4);
            }

            ic_state_.emote_icons.push_back(std::move(icon));
        }

        courtroom_ = std::make_unique<CourtroomWidget>(render);
        ic_chat_ = std::make_unique<ICChatWidget>(&ic_state_);
        emote_selector_ = std::make_unique<EmoteSelectorWidget>(&ic_state_);
        interjection_ = std::make_unique<InterjectionWidget>(&ic_state_);
        side_select_ = std::make_unique<SideSelectWidget>(&ic_state_);
        message_options_ = std::make_unique<MessageOptionsWidget>(&ic_state_);
    }
}
