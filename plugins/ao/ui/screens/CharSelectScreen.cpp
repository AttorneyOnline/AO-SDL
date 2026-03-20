#include "ao/ui/screens/CharSelectScreen.h"

#include "asset/MediaManager.h"
#include "event/CharSelectRequestEvent.h"
#include "event/CharacterListEvent.h"
#include "event/CharsCheckEvent.h"
#include "event/EventManager.h"
#include "event/UIEvent.h"
#include "ao/ui/screens/CourtroomScreen.h"
#include "utils/Log.h"

#include <format>

void CharSelectScreen::enter(ScreenController& ctrl) {
    controller = &ctrl;
}

void CharSelectScreen::exit() {
    chars.clear();
    controller = nullptr;
}

void CharSelectScreen::handle_events() {
    // Receive character list from the network
    auto& char_list_channel = EventManager::instance().get_channel<CharacterListEvent>();
    while (auto optev = char_list_channel.get_event()) {
        chars.clear();
        for (const auto& folder : optev->get_characters()) {
            chars.push_back({folder, std::nullopt, false});
        }
        load_icons();
    }

    // Update taken status
    auto& chars_check_channel = EventManager::instance().get_channel<CharsCheckEvent>();
    while (auto optev = chars_check_channel.get_event()) {
        const auto& taken = optev->get_taken();
        for (size_t i = 0; i < taken.size() && i < chars.size(); i++) {
            chars[i].taken = taken[i];
        }
    }

    // Transition to courtroom on confirmed character selection
    auto& ui_channel = EventManager::instance().get_channel<UIEvent>();
    while (auto optev = ui_channel.get_event()) {
        if (optev->get_type() == UIEventType::ENTERED_COURTROOM) {
            controller->push_screen(
                std::make_unique<CourtroomScreen>(optev->get_character_name(), optev->get_char_id()));
        }
    }

}

void CharSelectScreen::select_character(int index) {
    if (index < 0 || index >= (int)chars.size())
        return;
    if (chars[index].taken)
        return;

    selected = index;
    EventManager::instance().get_channel<CharSelectRequestEvent>().publish(CharSelectRequestEvent(index));
}

void CharSelectScreen::load_icons() {
    AssetLibrary& lib = MediaManager::instance().assets();

    for (auto& entry : chars) {
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
