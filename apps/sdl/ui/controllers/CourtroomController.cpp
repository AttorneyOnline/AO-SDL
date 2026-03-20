#include "ui/controllers/CourtroomController.h"

#include "ao/asset/AOAssetLibrary.h"
#include "ao/ui/screens/CourtroomScreen.h"
#include "asset/MediaManager.h"

#include <imgui.h>

static int side_to_index(const std::string& side) {
    static constexpr const char* SIDES[] = {"def", "pro", "wit", "jud", "jur", "sea", "hlp"};
    for (int i = 0; i < 7; i++) {
        if (side == SIDES[i])
            return i;
    }
    return 2; // default: wit
}

CourtroomController::CourtroomController(CourtroomScreen& screen, RenderManager& render) {
    ic_state_ = {};
    ic_state_.character = screen.get_character_name();
    ic_state_.char_id = screen.get_char_id();

    AOAssetLibrary ao_assets(MediaManager::instance().assets());
    ic_state_.char_sheet = ao_assets.character_sheet(ic_state_.character);

    if (ic_state_.char_sheet) {
        ic_state_.side_index = side_to_index(ic_state_.char_sheet->side());
        std::strncpy(ic_state_.showname, ic_state_.char_sheet->showname().c_str(),
                     sizeof(ic_state_.showname) - 1);
    }

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

void CourtroomController::render() {
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
        nav_action_ = IUIRenderer::NavAction::POP_SCREEN;
    ImGui::SameLine();
    if (ImGui::Button("Disconnect"))
        nav_action_ = IUIRenderer::NavAction::POP_TO_ROOT;
    ImGui::End();
}

IUIRenderer::NavAction CourtroomController::nav_action() {
    auto action = nav_action_;
    nav_action_ = IUIRenderer::NavAction::NONE;
    return action;
}
