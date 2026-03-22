#include "ao/ui/screens/CharSelectScreen.h"

#include "ao/ui/screens/CourtroomScreen.h"
#include "asset/MediaManager.h"
#include "event/CharSelectRequestEvent.h"
#include "event/CharacterListEvent.h"
#include "event/CharsCheckEvent.h"
#include "event/EventManager.h"
#include "event/UIEvent.h"

#include <format>

void CharSelectScreen::enter(ScreenController& ctrl) {
    controller = &ctrl;
    // Re-entering from courtroom: re-request prefetches that were dropped
    prefetch_cursor_ = 0;
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
        prefetch_cursor_ = 0;
    }

    // Update taken status
    auto& chars_check_channel = EventManager::instance().get_channel<CharsCheckEvent>();
    while (auto optev = chars_check_channel.get_event()) {
        const auto& taken = optev->get_taken();
        for (size_t i = 0; i < taken.size() && i < chars.size(); i++) {
            chars[i].taken = taken[i];
        }
    }

    // Progressively load icons: prefetch + decode + upload, all batched
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
    if (chars[index].taken && index != selected)
        return;

    if (index == selected && controller) {
        controller->push_screen(std::make_unique<CourtroomScreen>(chars[index].folder, index));
        return;
    }

    selected = index;
    EventManager::instance().get_channel<CharSelectRequestEvent>().publish(CharSelectRequestEvent(index));
}

void CharSelectScreen::retry_icons() {
    AssetLibrary& lib = MediaManager::instance().assets();

    // Drip-feed HTTP prefetch requests across frames.
    // With keep-alive connections this is just queue insertion — cheap.
    for (int i = 0; i < 32 && prefetch_cursor_ < (int)chars.size(); ++i, ++prefetch_cursor_) {
        std::string icon_path = std::format("characters/{}/char_icon", chars[prefetch_cursor_].folder);
        lib.prefetch_image(icon_path, 0, 0);
    }

    // Re-request icons that failed transiently (timeout, etc.) on initial pass.
    // MountHttp::request() is a no-op for cached, pending, or permanently failed
    // paths, so this is cheap once everything has settled. Reset the cursor on
    // each complete scan to pick up icons whose transient failures have cleared.
    if (prefetch_cursor_ >= (int)chars.size()) {
        if (retry_cursor_ >= (int)chars.size())
            retry_cursor_ = 0;
        for (int i = 0; i < 16 && retry_cursor_ < (int)chars.size(); ++i, ++retry_cursor_) {
            if (!chars[retry_cursor_].icon.has_value()) {
                std::string icon_path = std::format("characters/{}/char_icon", chars[retry_cursor_].folder);
                lib.prefetch_image(icon_path, 0, 0);
            }
        }
    }

    // Decode + GPU upload, batched to avoid GL driver stalls.
    // Scan from the start each time since icons arrive out of order.
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
