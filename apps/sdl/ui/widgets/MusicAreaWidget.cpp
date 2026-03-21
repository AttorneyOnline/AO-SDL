#include "ui/widgets/MusicAreaWidget.h"

#include "ui/widgets/ICMessageState.h"

#include "event/AreaUpdateEvent.h"
#include "event/EventManager.h"
#include "event/MusicChangeEvent.h"
#include "event/MusicListEvent.h"
#include "event/OutgoingMusicEvent.h"

#include <imgui.h>

#include <algorithm>

void MusicAreaWidget::handle_events() {
    auto& list_ch = EventManager::instance().get_channel<MusicListEvent>();
    while (auto ev = list_ch.get_event()) {
        areas_ = ev->areas();
        tracks_ = ev->tracks();
        // Pre-lowercase track names for filtering
        tracks_lower_.resize(tracks_.size());
        for (size_t i = 0; i < tracks_.size(); i++) {
            tracks_lower_[i] = tracks_[i];
            std::transform(tracks_lower_[i].begin(), tracks_lower_[i].end(),
                           tracks_lower_[i].begin(), [](unsigned char c) { return std::tolower(c); });
        }
        // Reset ARUP data to match new area count
        size_t n = areas_.size();
        area_players_.assign(n, -1);
        area_status_.assign(n, "Unknown");
        area_cm_.assign(n, "Unknown");
        area_lock_.assign(n, "Unknown");
    }

    auto& arup_ch = EventManager::instance().get_channel<AreaUpdateEvent>();
    while (auto ev = arup_ch.get_event()) {
        const auto& vals = ev->values();
        size_t count = std::min(vals.size(), areas_.size());
        switch (ev->type()) {
        case AreaUpdateEvent::PLAYERS:
            for (size_t i = 0; i < count; i++)
                area_players_[i] = std::atoi(vals[i].c_str());
            break;
        case AreaUpdateEvent::STATUS:
            for (size_t i = 0; i < count; i++)
                area_status_[i] = vals[i];
            break;
        case AreaUpdateEvent::CM:
            for (size_t i = 0; i < count; i++)
                area_cm_[i] = vals[i];
            break;
        case AreaUpdateEvent::LOCK:
            for (size_t i = 0; i < count; i++)
                area_lock_[i] = vals[i];
            break;
        }
    }

    auto& change_ch = EventManager::instance().get_channel<MusicChangeEvent>();
    while (auto ev = change_ch.get_event()) {
        now_playing_ = ev->track();
    }
}

static bool matches_filter(const std::string& lower_name, const std::string& lower_filter) {
    if (lower_filter.empty())
        return true;
    return lower_name.find(lower_filter) != std::string::npos;
}

static ImVec4 status_color(const std::string& status) {
    if (status == "LOOKING-FOR-PLAYERS") return {0.56f, 0.93f, 0.56f, 1.0f};
    if (status == "CASING")              return {1.0f, 0.84f, 0.0f, 1.0f};
    if (status == "RECESS")              return {0.68f, 0.85f, 0.90f, 1.0f};
    if (status == "RP")                  return {0.87f, 0.63f, 0.87f, 1.0f};
    if (status == "GAMING")              return {1.0f, 0.65f, 0.0f, 1.0f};
    return {0.7f, 0.7f, 0.7f, 1.0f}; // default/unknown
}

void MusicAreaWidget::render() {
    if (ImGui::BeginTabBar("##music_area_tabs")) {
        if (ImGui::BeginTabItem("Music")) {
            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##music_search", "Search...", search_buf_, sizeof(search_buf_));

            if (!now_playing_.empty()) {
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Now: %s", now_playing_.c_str());
            }

            // Lowercase the filter once per frame
            std::string lower_filter(search_buf_);
            std::transform(lower_filter.begin(), lower_filter.end(), lower_filter.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            ImGui::BeginChild("##music_list", ImVec2(0, 0), ImGuiChildFlags_None);

            for (int i = 0; i < (int)tracks_.size(); i++) {
                const auto& track = tracks_[i];
                if (i < (int)tracks_lower_.size() && !matches_filter(tracks_lower_[i], lower_filter))
                    continue;

                bool is_category = !track.empty() && track.find('.') == std::string::npos;
                if (is_category) {
                    ImGui::SeparatorText(track.c_str());
                } else {
                    if (ImGui::Selectable(track.c_str())) {
                        std::string showname = state_->showname;
                        EventManager::instance().get_channel<OutgoingMusicEvent>().publish(
                            OutgoingMusicEvent(track, showname));
                    }
                }
            }

            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Areas")) {
            ImGui::BeginChild("##area_list", ImVec2(0, 0), ImGuiChildFlags_None);

            for (int i = 0; i < (int)areas_.size(); i++) {
                ImGui::PushID(i);
                bool selected = (i == selected_area_);
                bool locked = (i < (int)area_lock_.size() && area_lock_[i] == "LOCKED");

                if (locked)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.4f, 0.4f, 1.0f));

                if (ImGui::Selectable(areas_[i].c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                    selected_area_ = i;
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        std::string showname = state_->showname;
                        EventManager::instance().get_channel<OutgoingMusicEvent>().publish(
                            OutgoingMusicEvent(areas_[i], showname));
                    }
                }

                if (locked)
                    ImGui::PopStyleColor();

                // Area metadata line
                if (i < (int)area_status_.size()) {
                    ImGui::SameLine();
                    ImGui::TextColored(status_color(area_status_[i]), " [%s]", area_status_[i].c_str());

                    if (i < (int)area_players_.size() && area_players_[i] >= 0) {
                        ImGui::SameLine();
                        ImGui::TextDisabled("(%d)", area_players_[i]);
                    }

                    if (i < (int)area_cm_.size() && area_cm_[i] != "FREE" && area_cm_[i] != "Unknown") {
                        ImGui::SameLine();
                        ImGui::TextDisabled("CM: %s", area_cm_[i].c_str());
                    }
                }

                ImGui::PopID();
            }

            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
