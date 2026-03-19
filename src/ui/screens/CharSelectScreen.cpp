#include "CharSelectScreen.h"

#include "ui/screens/CourtroomScreen.h"
#include "asset/MediaManager.h"
#include "event/CharacterListEvent.h"
#include "event/CharSelectRequestEvent.h"
#include "event/CharsCheckEvent.h"
#include "event/EventManager.h"
#include "event/UIEvent.h"
#include "utils/Log.h"

#include <format>
#include <imgui.h>

void CharSelectScreen::enter(ScreenController& controller) {
    m_controller = &controller;
}

void CharSelectScreen::exit() {
    m_chars.clear();
    m_controller = nullptr;
}

void CharSelectScreen::handle_events() {
    // Receive character list from the network
    auto& char_list_channel = EventManager::instance().get_channel<CharacterListEvent>();
    while (auto optev = char_list_channel.get_event()) {
        m_chars.clear();
        for (const auto& folder : optev->get_characters()) {
            m_chars.push_back({folder, std::nullopt, false});
        }
        load_icons();
    }

    // Update taken status
    auto& chars_check_channel = EventManager::instance().get_channel<CharsCheckEvent>();
    while (auto optev = chars_check_channel.get_event()) {
        const auto& taken = optev->get_taken();
        for (size_t i = 0; i < taken.size() && i < m_chars.size(); i++) {
            m_chars[i].taken = taken[i];
        }
    }

    // Transition to courtroom on confirmed character selection
    auto& ui_channel = EventManager::instance().get_channel<UIEvent>();
    while (auto optev = ui_channel.get_event()) {
        if (optev->get_type() == UIEventType::ENTERED_COURTROOM) {
            m_controller->push_screen(std::make_unique<CourtroomScreen>());
        }
    }

    m_chat.handle_events();
}

void CharSelectScreen::load_icons() {
    AssetLibrary& lib = MediaManager::instance().assets();

    for (auto& entry : m_chars) {
        std::string icon_path = std::format("characters/{}/char_icon", entry.folder);
        auto asset = lib.image(icon_path);

        if (!asset || asset->frame_count() == 0) {
            Log::log_print(DEBUG, "No icon for character: %s", entry.folder.c_str());
            continue;
        }

        const ImageFrame& frame = asset->frame(0);
        entry.icon.emplace(frame.width, frame.height, frame.pixels.data(), 4);
    }
}

void CharSelectScreen::render(RenderManager& render) {
    render.begin_frame();

    ImGui::Begin("Character Select");

    if (m_chars.empty()) {
        ImGui::Text("Waiting for character list...");
    }
    else {
        const float icon_size = 64.0f;
        const float frame_padding = ImGui::GetStyle().FramePadding.x;
        const float item_spacing = ImGui::GetStyle().ItemSpacing.x;
        const float button_size = icon_size + frame_padding * 2.0f;
        const float panel_width = ImGui::GetContentRegionAvail().x;
        int columns = std::max(1, (int)((panel_width + item_spacing) / (button_size + item_spacing)));

        for (int i = 0; i < (int)m_chars.size(); i++) {
            const auto& entry = m_chars[i];

            ImGui::PushID(i);

            bool selected = (m_selected == i);
            if (selected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
            if (entry.taken)
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);

            bool clicked = false;
            if (entry.icon.has_value()) {
                ImTextureID tex = (ImTextureID)(uintptr_t)entry.icon->get_id();
                // uv0=(0,1) uv1=(1,0): flip V to correct for stb vertical flip convention
                clicked = ImGui::ImageButton("icon", tex, ImVec2(icon_size, icon_size), {0, 1}, {1, 0});
            }
            else {
                clicked = ImGui::Button(entry.folder.c_str(), ImVec2(icon_size, icon_size));
            }

            if (selected)   ImGui::PopStyleColor();
            if (entry.taken) ImGui::PopStyleColor();

            if (clicked && !entry.taken) {
                m_selected = i;
                EventManager::instance().get_channel<CharSelectRequestEvent>().publish(
                    CharSelectRequestEvent(i));
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s%s", entry.folder.c_str(), entry.taken ? " (taken)" : "");
            }

            if ((i + 1) % columns != 0 && i + 1 < (int)m_chars.size()) {
                ImGui::SameLine();
            }

            ImGui::PopID();
        }
    }

    ImGui::End();

    m_chat.render();

    ImGui::Render();
}
