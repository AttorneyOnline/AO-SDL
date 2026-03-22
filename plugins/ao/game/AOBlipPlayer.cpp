#include "ao/game/AOBlipPlayer.h"

#include "event/EventManager.h"
#include "event/PlayBlipEvent.h"
#include "utils/UTF8.h"

void AOBlipPlayer::start(AOAssetLibrary& ao_assets, const std::string& character) {
    blip_asset_ = ao_assets.character_blip(character);
    character_ = character;
    blip_ticker_ = 0;
}

void AOBlipPlayer::tick(AOAssetLibrary& ao_assets, int prev_chars, int cur_chars, const std::string& message,
                        bool talking, bool active) {
    if (prev_chars >= cur_chars || !talking || !active)
        return;

    // Retry loading blip if it wasn't available at start time (HTTP delay)
    if (!blip_asset_ && !character_.empty())
        blip_asset_ = ao_assets.character_blip(character_);
    if (!blip_asset_)
        return;

    for (int i = prev_chars; i < cur_chars; ++i) {
        size_t byte_pos = UTF8::byte_offset(message, i);
        if (byte_pos < message.size() && is_whitespace(message[byte_pos])) {
            blip_ticker_ = 0; // reset cadence after whitespace
            continue;
        }

        if (blip_ticker_ % BLIP_RATE == 0) {
            EventManager::instance().get_channel<PlayBlipEvent>().publish(PlayBlipEvent(blip_asset_, 1.0f));
        }
        ++blip_ticker_;
    }
}

bool AOBlipPlayer::is_whitespace(char c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}
