#include "ao/ui/screens/CourtroomScreen.h"

#include "ao/asset/AOAssetLibrary.h"
#include "asset/MediaManager.h"
#include "asset/MountManager.h"
#include "utils/Log.h"

#include <chrono>
#include <thread>

CourtroomScreen::CourtroomScreen(std::string character_name, int char_id)
    : character_name_(std::move(character_name)), char_id_(char_id) {
    // Drop low-priority char icon downloads — we're entering the courtroom now
    MediaManager::instance().mounts_ref().drop_http_below(1);

    // Kick off async loading so the UI thread isn't blocked
    load_future_ = std::async(std::launch::async, &CourtroomScreen::load_character_data, this);
}

void CourtroomScreen::load_character_data() {
    Log::log_print(DEBUG, "CourtroomScreen: loading character data for '%s'", character_name_.c_str());

    AOAssetLibrary ao_assets(MediaManager::instance().assets());

    // Prefetch char.ini — triggers async HTTP download
    ao_assets.prefetch_character(character_name_, "", "");

    // Poll until char.ini arrives or timeout (5 seconds)
    std::optional<AOCharacterSheet> sheet;
    for (int i = 0; i < 50; ++i) {
        sheet = ao_assets.character_sheet(character_name_);
        if (sheet)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (sheet)
        char_sheet_ = std::make_shared<AOCharacterSheet>(std::move(*sheet));

    // Prefetch all emote icons and sprites for our character via HTTP
    ao_assets.prefetch_own_character(character_name_);

    int count = char_sheet_ ? char_sheet_->emote_count() : 0;
    for (int i = 0; i < count; i++) {
        emote_icons_.push_back(ao_assets.emote_icon(character_name_, i));
    }

    Log::log_print(DEBUG, "CourtroomScreen: character data loaded (%d emotes)", count);
    load_generation_++;
    loading_ = false;
}

void CourtroomScreen::change_character(const std::string& character_name, int char_id) {
    // Wait for any in-flight load to finish before swapping
    if (load_future_.valid())
        load_future_.wait();

    character_name_ = character_name;
    char_id_ = char_id;
    loading_ = true;
    char_sheet_.reset();
    emote_icons_.clear();

    // Drop low-priority downloads from the old character
    MediaManager::instance().mounts_ref().drop_http_below(1);

    load_future_ = std::async(std::launch::async, &CourtroomScreen::load_character_data, this);
}

void CourtroomScreen::enter(ScreenController&) {
}

void CourtroomScreen::exit() {
    // Ensure the async load has completed before destruction
    if (load_future_.valid())
        load_future_.wait();
}

void CourtroomScreen::handle_events() {
}
