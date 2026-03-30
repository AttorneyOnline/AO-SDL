#include "ui/widgets/MusicAreaWidget.h"

#include "ui/widgets/CourtroomState.h"
#include "ui/widgets/ICMessageState.h"

#include "event/AreaUpdateEvent.h"
#include "event/EventManager.h"
#include "event/MusicListEvent.h"
#include "event/NowPlayingEvent.h"
#include "event/OutgoingMusicEvent.h"
#include "event/VolumeChangeEvent.h"

#include "utils/StringHelpers.h"

#include <imgui.h>

#include <algorithm>

void MusicAreaWidget::handle_events() {
    auto& cs = CourtroomState::instance();

    auto& list_ch = EventManager::instance().get_channel<MusicListEvent>();
    while (auto ev = list_ch.get_event()) {
        if (ev->partial()) {
            if (!ev->areas().empty()) {
                cs.areas = ev->areas();
                size_t n = cs.areas.size();
                cs.area_players.assign(n, -1);
                cs.area_status.assign(n, "Unknown");
                cs.area_cm.assign(n, "Unknown");
                cs.area_lock.assign(n, "Unknown");
            }
            if (!ev->tracks().empty()) {
                cs.tracks = ev->tracks();
            }
        }
        else {
            cs.areas = ev->areas();
            cs.tracks = ev->tracks();
            size_t n = cs.areas.size();
            cs.area_players.assign(n, -1);
            cs.area_status.assign(n, "Unknown");
            cs.area_cm.assign(n, "Unknown");
            cs.area_lock.assign(n, "Unknown");
        }
        tracks_lower_.resize(cs.tracks.size());
        for (size_t i = 0; i < cs.tracks.size(); i++) {
            tracks_lower_[i] = StringHelpers::trim_song_name(cs.tracks[i]);
            std::transform(tracks_lower_[i].begin(), tracks_lower_[i].end(), tracks_lower_[i].begin(),
                           [](unsigned char c) { return std::tolower(c); });
        }
    }

    auto& arup_ch = EventManager::instance().get_channel<AreaUpdateEvent>();
    while (auto ev = arup_ch.get_event()) {
        const auto& vals = ev->values();
        size_t count = std::min(vals.size(), cs.areas.size());
        switch (ev->type()) {
        case AreaUpdateEvent::PLAYERS:
            for (size_t i = 0; i < count; i++)
                cs.area_players[i] = std::atoi(vals[i].c_str());
            break;
        case AreaUpdateEvent::STATUS:
            for (size_t i = 0; i < count; i++)
                cs.area_status[i] = vals[i];
            break;
        case AreaUpdateEvent::CM:
            for (size_t i = 0; i < count; i++)
                cs.area_cm[i] = vals[i];
            break;
        case AreaUpdateEvent::LOCK:
            for (size_t i = 0; i < count; i++)
                cs.area_lock[i] = vals[i];
            break;
        }
    }

    auto& now_ch = EventManager::instance().get_channel<NowPlayingEvent>();
    while (auto ev = now_ch.get_event()) {
        cs.now_playing = ev->track();
    }
}

static bool matches_filter(const std::string& lower_name, const std::string& lower_filter) {
    if (lower_filter.empty())
        return true;
    return lower_name.find(lower_filter) != std::string::npos;
}

static ImVec4 status_color(const std::string& status) {
    if (status == "LOOKING-FOR-PLAYERS")
        return {0.56f, 0.93f, 0.56f, 1.0f};
    if (status == "CASING")
        return {1.0f, 0.84f, 0.0f, 1.0f};
    if (status == "RECESS")
        return {0.68f, 0.85f, 0.90f, 1.0f};
    if (status == "RP")
        return {0.87f, 0.63f, 0.87f, 1.0f};
    if (status == "GAMING")
        return {1.0f, 0.65f, 0.0f, 1.0f};
    return {0.7f, 0.7f, 0.7f, 1.0f}; // default/unknown
}

void MusicAreaWidget::render() {
    auto& cs = CourtroomState::instance();

    if (ImGui::BeginTabBar("##music_area_tabs")) {
        if (ImGui::BeginTabItem("Music")) {
            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##music_search", "Search...", search_buf_, sizeof(search_buf_));

            if (!cs.now_playing.empty()) {
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Now: %s",
                                   StringHelpers::trim_song_name(cs.now_playing).c_str());
            }

            std::string lower_filter(search_buf_);
            std::transform(lower_filter.begin(), lower_filter.end(), lower_filter.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            ImGui::BeginChild("##music_list", ImVec2(0, 0), ImGuiChildFlags_None);

            std::string current_category;
            bool tree_open = false;
            bool category_has_match = false;
            bool category_is_match = false;

            auto is_category_fn = [](const std::string& t) { return !t.empty() && t.find('.') == std::string::npos; };

            for (int i = 0; i < (int)cs.tracks.size(); i++) {
                const auto& item = cs.tracks[i];

                if (is_category_fn(item)) {
                    if (tree_open) {
                        ImGui::TreePop();
                        tree_open = false;
                    }

                    current_category = item;

                    // we want to draw any categories which have tracks in them that match the filter
                    // we also want to draw every song in a category which itself matches the filter
                    category_is_match = false;
                    category_has_match = false;
                    for (int j = i + 1; j < (int)cs.tracks.size(); j++) {
                        if (is_category_fn(cs.tracks[j]))
                            break;

                        if (matches_filter(tracks_lower_[i], lower_filter)) {
                            category_is_match = true;
                            break;
                        }

                        if (j < (int)tracks_lower_.size() && matches_filter(tracks_lower_[j], lower_filter)) {
                            category_has_match = true;
                            break;
                        }
                    }

                    if (lower_filter.empty() || category_is_match || category_has_match) {
                        tree_open = ImGui::TreeNode(current_category.c_str());
                    }
                    else {
                        tree_open = false;
                    }
                }
                else {
                    bool matches = category_is_match ||
                                   ((i < (int)tracks_lower_.size()) && matches_filter(tracks_lower_[i], lower_filter));

                    if (tree_open && (lower_filter.empty() || matches)) {
                        if (ImGui::Selectable(StringHelpers::trim_song_name(item).c_str())) {
                            std::string showname = state_->showname;
                            EventManager::instance().get_channel<OutgoingMusicEvent>().publish(
                                OutgoingMusicEvent(item, showname));
                        }
                    }
                }
            }

            if (tree_open) {
                ImGui::TreePop();
            }

            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Areas")) {
            ImGui::BeginChild("##area_list", ImVec2(0, 0), ImGuiChildFlags_None);

            for (int i = 0; i < (int)cs.areas.size(); i++) {
                ImGui::PushID(i);
                bool selected = (i == selected_area_);
                bool locked = (i < (int)cs.area_lock.size() && cs.area_lock[i] == "LOCKED");

                if (locked)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.4f, 0.4f, 1.0f));

                if (ImGui::Selectable(cs.areas[i].c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                    selected_area_ = i;
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        std::string showname = state_->showname;
                        EventManager::instance().get_channel<OutgoingMusicEvent>().publish(
                            OutgoingMusicEvent(cs.areas[i], showname));
                    }
                }

                if (locked)
                    ImGui::PopStyleColor();

                if (i < (int)cs.area_status.size()) {
                    ImGui::SameLine();
                    ImGui::TextColored(status_color(cs.area_status[i]), " [%s]", cs.area_status[i].c_str());

                    if (i < (int)cs.area_players.size() && cs.area_players[i] >= 0) {
                        ImGui::SameLine();
                        ImGui::TextDisabled("(%d)", cs.area_players[i]);
                    }

                    if (i < (int)cs.area_cm.size() && cs.area_cm[i] != "FREE" && cs.area_cm[i] != "Unknown") {
                        ImGui::SameLine();
                        ImGui::TextDisabled("CM: %s", cs.area_cm[i].c_str());
                    }
                }

                ImGui::PopID();
            }

            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Volume")) {
            // Cubic curve: slider 50% → amplitude 0.125 (≈ -18dB).
            auto to_amplitude = [](int pct) -> float {
                float t = pct / 100.0f;
                return t * t * t;
            };

            float w = ImGui::GetContentRegionAvail().x - 50;
            ImGui::SetNextItemWidth(w);
            if (ImGui::SliderInt("Music", &music_vol_, 0, 100)) {
                EventManager::instance().get_channel<VolumeChangeEvent>().publish(
                    VolumeChangeEvent(VolumeChangeEvent::Category::MUSIC, to_amplitude(music_vol_)));
            }
            ImGui::SetNextItemWidth(w);
            if (ImGui::SliderInt("SFX", &sfx_vol_, 0, 100)) {
                EventManager::instance().get_channel<VolumeChangeEvent>().publish(
                    VolumeChangeEvent(VolumeChangeEvent::Category::SFX, to_amplitude(sfx_vol_)));
            }
            ImGui::SetNextItemWidth(w);
            if (ImGui::SliderInt("Blips", &blip_vol_, 0, 100)) {
                EventManager::instance().get_channel<VolumeChangeEvent>().publish(
                    VolumeChangeEvent(VolumeChangeEvent::Category::BLIP, to_amplitude(blip_vol_)));
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
