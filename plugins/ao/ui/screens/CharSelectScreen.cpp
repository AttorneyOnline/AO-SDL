#include "ao/ui/screens/CharSelectScreen.h"

#include "ao/ui/screens/CourtroomScreen.h"
#include "asset/MediaManager.h"
#include "asset/MountManager.h"
#include "event/CharSelectRequestEvent.h"
#include "event/CharacterListEvent.h"
#include "event/CharsCheckEvent.h"
#include "event/EventManager.h"
#include "event/UIEvent.h"

#include <cstdint>
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
        icon_probe_failed_.clear();
        for (const auto& folder : optev->get_characters()) {
            chars.push_back({folder, std::nullopt, nullptr, false});
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
    // Use server-advertised charicon extensions for the initial pass.
    auto exts = MediaManager::instance().mounts_ref().http_extensions(0); // 0 = charicon
    if (exts.empty())
        exts = {"webp", "apng", "gif", "png"};

    for (int i = 0; i < 32 && prefetch_cursor_ < (int)chars.size(); ++i, ++prefetch_cursor_) {
        std::string icon_path = std::format("characters/{}/char_icon", chars[prefetch_cursor_].folder);
        lib.prefetch(icon_path, exts, 0);
    }

    // Re-request icons that failed transiently (timeout, etc.) on initial pass.
    // Use the same server-advertised extensions — icons either exist in that
    // format or don't exist at all (unlike emotes which may use unlisted formats).
    if (prefetch_cursor_ >= (int)chars.size()) {
        if (retry_cursor_ >= (int)chars.size())
            retry_cursor_ = 0;
        for (int i = 0; i < 16 && retry_cursor_ < (int)chars.size(); ++i, ++retry_cursor_) {
            if (!chars[retry_cursor_].icon.has_value()) {
                std::string icon_path = std::format("characters/{}/char_icon", chars[retry_cursor_].folder);
                lib.prefetch(icon_path, exts, 0);
            }
        }
    }

    // Decode + GPU upload — only scan when new HTTP data has arrived.
    uint32_t gen = MediaManager::instance().mounts_ref().http_cache_generation();
    if (gen != last_http_gen_) {
        icon_probe_failed_.clear();
        last_http_gen_ = gen;
    }

    int uploaded = 0;
    for (int i = 0; i < (int)chars.size(); ++i) {
        auto& entry = chars[i];
        if (entry.icon.has_value() || icon_probe_failed_.count(i))
            continue;

        std::string icon_path = std::format("characters/{}/char_icon", entry.folder);
        auto asset = lib.image(icon_path);
        if (!asset || asset->frame_count() == 0) {
            icon_probe_failed_.insert(i);
            continue;
        }

        const ImageFrame& frame = asset->frame(0);
        entry.icon.emplace(frame.width, frame.height, asset->frame_pixels(0), 4);
        entry.icon_asset = asset; // Retain for pixel access (Flutter FFI)
        if (++uploaded >= 8)
            break;
    }
}
