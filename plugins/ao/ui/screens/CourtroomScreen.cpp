#include "ao/ui/screens/CourtroomScreen.h"

#include "ao/asset/AOAssetLibrary.h"
#include "asset/MediaManager.h"

CourtroomScreen::CourtroomScreen(std::string character_name, int char_id)
    : character_name_(std::move(character_name)), char_id_(char_id) {
    // Load character data here in the plugin layer so the app/UI layer
    // doesn't need to know about AOAssetLibrary.
    AOAssetLibrary ao_assets(MediaManager::instance().assets());
    auto sheet = ao_assets.character_sheet(character_name_);
    if (sheet)
        char_sheet_ = std::make_shared<AOCharacterSheet>(std::move(*sheet));

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
