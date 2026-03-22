#include "ao/ui/screens/CourtroomScreen.h"

#include "ao/asset/AOAssetLibrary.h"
#include "asset/MediaManager.h"
#include "asset/MountManager.h"
#include "utils/Log.h"

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
    auto sheet = ao_assets.character_sheet(character_name_);
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

void CourtroomScreen::enter(ScreenController&) {
}

void CourtroomScreen::exit() {
    // Ensure the async load has completed before destruction
    if (load_future_.valid())
        load_future_.wait();
}

void CourtroomScreen::handle_events() {
}
