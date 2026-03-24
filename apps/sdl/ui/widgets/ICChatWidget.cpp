#include "ui/widgets/ICChatWidget.h"

#include "ui/widgets/ICMessageState.h"

#include "event/EventManager.h"
#include "event/OutgoingICMessageEvent.h"

#include <imgui.h>

static constexpr const char* SIDES[] = {"def", "pro", "wit", "jud", "jur", "sea", "hlp"};

void ICChatWidget::handle_events() {
}

void ICChatWidget::send() {
    ICMessageData data;
    data.character = state_->character;
    data.char_id = state_->char_id;
    data.message = state_->message;
    data.showname = state_->showname;

    if (state_->char_sheet && state_->selected_emote >= 0 &&
        state_->selected_emote < state_->char_sheet->emote_count()) {
        const auto& emo = state_->char_sheet->emote(state_->selected_emote);
        data.emote = emo.anim_name;
        data.pre_emote = emo.pre_anim;
        data.desk_mod = emo.desk_mod;
        if (!emo.sfx_name.empty() && emo.sfx_name != "0") {
            data.sfx_name = emo.sfx_name;
            data.sfx_delay = emo.sfx_delay;
        }
    }

    data.emote_mod = state_->pre_anim ? 1 : 0;
    data.side = SIDES[state_->side_index];
    data.objection_mod = state_->objection_mod;
    data.flip = state_->flip ? 1 : 0;
    data.text_color = state_->text_color;
    data.realization = state_->realization ? 1 : 0;
    data.screenshake = state_->screenshake ? 1 : 0;
    data.additive = state_->additive ? 1 : 0;
    data.slide = state_->slide ? "1" : "0";

    EventManager::instance().get_channel<OutgoingICMessageEvent>().publish(OutgoingICMessageEvent(std::move(data)));

    state_->message[0] = '\0';
    state_->objection_mod = 0;
    state_->realization = false;
    state_->screenshake = false;
    refocus_ = true;
}

void ICChatWidget::render() {
    ImGui::InputText("Showname", state_->showname, sizeof(state_->showname));

    if (refocus_) {
        ImGui::SetKeyboardFocusHere();
        refocus_ = false;
    }

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Send").x -
                            ImGui::GetStyle().FramePadding.x * 2 - ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::InputText("##ic_msg", state_->message, sizeof(state_->message), ImGuiInputTextFlags_EnterReturnsTrue)) {
        send();
    }

    ImGui::SameLine();
    if (ImGui::Button("Send")) {
        send();
    }
}
