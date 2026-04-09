#include "ui/widgets/PlayerListWidget.h"

#include "ui/widgets/CourtroomState.h"

#include "event/EventManager.h"
#include "event/PlayerListEvent.h"

#include <imgui.h>

void PlayerListWidget::handle_events() {
    auto& cs = CourtroomState::instance();
    auto& ch = EventManager::instance().get_channel<PlayerListEvent>();
    while (auto ev = ch.get_event()) {
        int id = ev->player_id();
        switch (ev->action()) {
        case PlayerListEvent::Action::ADD:
            cs.players[id] = {};
            break;
        case PlayerListEvent::Action::REMOVE:
            cs.players.erase(id);
            break;
        case PlayerListEvent::Action::UPDATE_NAME:
            cs.players[id].name = ev->data();
            break;
        case PlayerListEvent::Action::UPDATE_CHARACTER:
            cs.players[id].character = ev->data();
            break;
        case PlayerListEvent::Action::UPDATE_CHARNAME:
            cs.players[id].charname = ev->data();
            break;
        case PlayerListEvent::Action::UPDATE_AREA: {
            int area_id = std::atoi(ev->data().c_str());
            cs.players[id].area_id = area_id;
            // Track our own area
            if (id == cs.local_player_id)
                cs.current_area_id = area_id;
            break;
        }
        }
    }
}

void PlayerListWidget::render() {
    auto& cs = CourtroomState::instance();
    if (cs.players.empty()) {
        ImGui::TextDisabled("No players");
        return;
    }

    // Count players in current area vs total
    int in_area = 0;
    for (const auto& [id, info] : cs.players) {
        if (cs.current_area_id >= 0 && info.area_id == cs.current_area_id)
            ++in_area;
    }

    // Filter toggle
    ImGui::Checkbox("This area only", &filter_by_area_);
    ImGui::SameLine();
    if (filter_by_area_ && cs.current_area_id >= 0)
        ImGui::Text("(%d here, %zu total)", in_area, cs.players.size());
    else
        ImGui::Text("(%zu online)", cs.players.size());

    ImGui::BeginChild("##player_list", ImVec2(0, 0), ImGuiChildFlags_None);

    for (const auto& [id, info] : cs.players) {
        // Area filter
        if (filter_by_area_ && cs.current_area_id >= 0 && info.area_id != cs.current_area_id)
            continue;

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

        // Show area name for players in other areas (when not filtering)
        if (!filter_by_area_ && info.area_id >= 0 && info.area_id < static_cast<int>(cs.areas.size()) &&
            info.area_id != cs.current_area_id) {
            ImGui::BulletText("%s", display.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("[%s]", cs.areas[info.area_id].c_str());
        }
        else {
            ImGui::BulletText("%s", display.c_str());
        }
    }

    ImGui::EndChild();
}
