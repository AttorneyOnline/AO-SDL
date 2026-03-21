#include "ao/ui/screens/CourtroomScreen.h"

#include "ao/asset/AOAssetLibrary.h"
#include "asset/MediaManager.h"
#include "asset/MountManager.h"

CourtroomScreen::CourtroomScreen(std::string character_name, int char_id)
    : character_name_(std::move(character_name)), char_id_(char_id) {
    // Load character data here in the plugin layer so the app/UI layer
    // doesn't need to know about AOAssetLibrary.
    // Drop low-priority char icon downloads — we're entering the courtroom now
    MediaManager::instance().mounts_ref().drop_http_below(1);

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
}

void CourtroomScreen::enter(ScreenController&) {
}

void CourtroomScreen::exit() {
}

void CourtroomScreen::handle_events() {
}
