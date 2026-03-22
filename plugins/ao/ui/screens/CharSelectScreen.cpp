#include "ao/ui/screens/CharSelectScreen.h"

#include "ao/ui/screens/CourtroomScreen.h"
#include "asset/MediaManager.h"
#include "event/CharSelectRequestEvent.h"
#include "event/CharacterListEvent.h"
#include "event/CharsCheckEvent.h"
#include "event/EventManager.h"
#include "event/UIEvent.h"
#include "utils/Log.h"

#include <format>

void CharSelectScreen::enter(ScreenController& ctrl) {
    controller = &ctrl;
    // Re-entering from courtroom: re-prefetch and reload icons.
    // The courtroom drops low-priority downloads on entry, so icons
    // that were still in-flight need to be re-requested.
    if (!chars.empty())
        load_icons();
}

void CharSelectScreen::exit() {
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

    // Retry loading icons that are pending HTTP download
    retry_icons();

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
    // Allow re-selecting our own character (already selected), reject others' taken chars
    if (chars[index].taken && index != selected)
        return;

    // If re-selecting the same character, just go back to courtroom without a state change
    if (index == selected && controller) {
        controller->push_screen(std::make_unique<CourtroomScreen>(chars[index].folder, index));
        return;
    }

    selected = index;
    EventManager::instance().get_channel<CharSelectRequestEvent>().publish(CharSelectRequestEvent(index));
}

void CharSelectScreen::load_icons() {
    AssetLibrary& lib = MediaManager::instance().assets();

    for (auto& entry : chars) {
        std::string icon_path = std::format("characters/{}/char_icon", entry.folder);

        // Trigger HTTP prefetch for missing icons (0=CHARICON, LOW priority)
        lib.prefetch_image(icon_path, 0, 0);

        auto asset = lib.image(icon_path);
        if (!asset || asset->frame_count() == 0)
            continue;

        const ImageFrame& frame = asset->frame(0);
        entry.icon.emplace(frame.width, frame.height, frame.pixels.data(), 4);
    }
}

void CharSelectScreen::retry_icons() {
    AssetLibrary& lib = MediaManager::instance().assets();

    // Limit texture uploads per frame to avoid GPU stalls on GL backends
    int uploaded = 0;
    for (auto& entry : chars) {
        if (entry.icon.has_value())
            continue;

        std::string icon_path = std::format("characters/{}/char_icon", entry.folder);
        auto asset = lib.image(icon_path);
        if (!asset || asset->frame_count() == 0)
            continue;

        const ImageFrame& frame = asset->frame(0);
        entry.icon.emplace(frame.width, frame.height, frame.pixels.data(), 4);
        if (++uploaded >= 8)
            break;
    }
}
