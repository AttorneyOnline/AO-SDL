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
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("##courtroom_screen", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float right_width = 280.0f;
    float left_width = avail.x - right_width - ImGui::GetStyle().ItemSpacing.x;
    float top_height = avail.y * 0.65f;
    float bottom_height = avail.y - top_height - ImGui::GetStyle().ItemSpacing.y;

    // === Top row ===

    // Top-left: courtroom viewport
    ImGui::BeginChild("##viewport", ImVec2(left_width, top_height), ImGuiChildFlags_Borders);
    courtroom_->render();
    ImGui::EndChild();

    ImGui::SameLine();

    // Top-right: emotes
    ImGui::BeginChild("##emotes", ImVec2(right_width, top_height), ImGuiChildFlags_Borders);
    ImGui::SeparatorText("Emotes");
    emote_selector_->render();
    ImGui::EndChild();

    // === Bottom row ===

    // Bottom-left: IC controls + chat
    float ic_controls_width = left_width * 0.55f;
    float ooc_width = left_width - ic_controls_width - ImGui::GetStyle().ItemSpacing.x;

    ImGui::BeginChild("##ic_controls", ImVec2(ic_controls_width, bottom_height), ImGuiChildFlags_Borders);
    ImGui::SeparatorText("IC Chat");
    ic_chat_->render();

    ImGui::Spacing();
    ImGui::SeparatorText("Interjections");
    interjection_->render();

    ImGui::Spacing();
    ImGui::SeparatorText("Position");
    side_select_->render();

    ImGui::Spacing();
    ImGui::SeparatorText("Options");
    message_options_->render();

    ImGui::Spacing();
    if (ImGui::Button("Change Character"))
        nav_action_ = IUIRenderer::NavAction::POP_SCREEN;
    ImGui::SameLine();
    if (ImGui::Button("Disconnect"))
        nav_action_ = IUIRenderer::NavAction::POP_TO_ROOT;
    ImGui::EndChild();

    ImGui::SameLine();

    // Bottom-right: OOC chat
    ImGui::BeginChild("##ooc_chat", ImVec2(ooc_width, bottom_height), ImGuiChildFlags_Borders);
    ImGui::SeparatorText("OOC Chat");
    chat_.handle_events();
    chat_.render();
    ImGui::EndChild();

    ImGui::End();
}

IUIRenderer::NavAction CourtroomController::nav_action() {
    auto action = nav_action_;
    nav_action_ = IUIRenderer::NavAction::NONE;
    return action;
}
