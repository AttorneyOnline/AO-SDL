#include "ui/widgets/PlayerListWidget.h"

#include "event/EventManager.h"
#include "event/PlayerListEvent.h"

#include <imgui.h>

void PlayerListWidget::handle_events() {
    auto& ch = EventManager::instance().get_channel<PlayerListEvent>();
    while (auto ev = ch.get_event()) {
        int id = ev->player_id();
        switch (ev->action()) {
        case PlayerListEvent::Action::ADD:
            players_[id] = {};
            break;
        case PlayerListEvent::Action::REMOVE:
            players_.erase(id);
            break;
        case PlayerListEvent::Action::UPDATE_NAME:
            players_[id].name = ev->data();
            break;
        case PlayerListEvent::Action::UPDATE_CHARACTER:
            players_[id].character = ev->data();
            break;
        case PlayerListEvent::Action::UPDATE_CHARNAME:
            players_[id].charname = ev->data();
            break;
        case PlayerListEvent::Action::UPDATE_AREA:
            players_[id].area_id = std::atoi(ev->data().c_str());
            break;
        }
    }
}

void PlayerListWidget::render() {
    if (players_.empty()) {
        ImGui::TextDisabled("No players");
        return;
    }

    ImGui::Text("%zu players online", players_.size());
    ImGui::BeginChild("##player_list", ImVec2(0, 0), ImGuiChildFlags_None);

    for (const auto& [id, info] : players_) {
        std::string display;
        if (!info.charname.empty())
            display = info.charname;
        else if (!info.character.empty())
            display = info.character;

        if (!info.name.empty()) {
            if (display.empty())
                display = info.name;
            else
                display += " (" + info.name + ")";
        }

        if (display.empty())
            display = "Player " + std::to_string(id);

        ImGui::BulletText("%s", display.c_str());
    }

    ImGui::EndChild();
}
