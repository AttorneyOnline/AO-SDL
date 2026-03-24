#include "ao/game/AOMessagePlayer.h"

#include "ao/event/ICLogEvent.h"
#include "ao/game/AOTextBox.h"
#include "event/EventManager.h"
#include "event/PlaySFXEvent.h"

AOMessagePlayer::Result AOMessagePlayer::play(const ICMessage& msg, AOAssetLibrary& ao_assets, AOTextBox& textbox,
                                              bool active) {
    Result result;
    auto& ic = result.ic;
    ic.position = msg.side;

    // Desk visibility
    if (msg.desk_mod == DeskMod::CHAT) {
        ic.show_desk = (msg.side == "def" || msg.side == "pro" || msg.side == "wit");
    }
    else {
        ic.show_desk = (msg.desk_mod == DeskMod::SHOW || msg.desk_mod == DeskMod::EMOTE_ONLY ||
                        msg.desk_mod == DeskMod::EMOTE_ONLY_EX);
    }
    ic.flip = msg.flip;

    // Prefetch character assets via HTTP
    ao_assets.prefetch_character(msg.character, msg.emote, msg.pre_emote, 2);

    // Load the character sheet so char.ini is promoted from the HTTP raw cache
    // into the asset cache (survives release_all_http eviction).
    auto sheet = ao_assets.character_sheet(msg.character);

    // Resolve showname: prefer the one from the message, fall back to char.ini
    std::string showname = msg.showname;
    if (showname.empty())
        showname = sheet ? sheet->showname() : msg.character;
    result.resolved_showname = showname;

    // Check if message text is blank (whitespace-only)
    bool blank = true;
    for (char c : msg.message) {
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            blank = false;
            break;
        }
    }

    // Start emote + textbox
    if (msg.message.empty() || blank) {
        textbox.start_message(showname, msg.message, msg.text_color, ao_assets.text_colors(), msg.additive);
        ic.emote_player.start(ao_assets, msg.character, msg.emote, "", EmoteMod::IDLE);
        ic.emote_player.transition_to_idle();
    }
    else {
        ic.emote_player.start(ao_assets, msg.character, msg.emote, msg.pre_emote, msg.emote_mod);

        // Blocking preanim: defer textbox until preanim finishes
        if (ic.emote_player.state() == AOEmotePlayer::State::PREANIM && !msg.immediate) {
            ic.preanim_blocking = true;
            ic.pending_showname = showname;
            ic.pending_message = msg.message;
            ic.pending_text_color = msg.text_color;
            ic.pending_additive = msg.additive;
            textbox.start_message("", "", 0, ao_assets.text_colors());
        }
        else {
            textbox.start_message(showname, msg.message, msg.text_color, ao_assets.text_colors(), msg.additive);
        }
    }

    // Effect triggers (names match presenter's NamedEffect keys)
    if (msg.screenshake)
        result.effects.push_back("screenshake");
    if (msg.realization)
        result.effects.push_back("flash");
    if (msg.message.find("rainbow") != std::string::npos)
        result.effects.push_back("rainbow");
    if (msg.message.find("glass") != std::string::npos)
        result.effects.push_back("shatter");
    if (msg.message.find("cube") != std::string::npos)
        result.effects.push_back("cube");

    // Resolve SFX: use packet sfx_name, fall back to char.ini emote SFX
    std::string sfx_name = msg.sfx_name;
    bool sfx_looping = msg.sfx_looping;
    if ((sfx_name.empty() || sfx_name == "0" || sfx_name == "1") && sheet) {
        auto* emote_entry = sheet->find_emote(msg.emote);
        if (emote_entry && !emote_entry->sfx_name.empty() && emote_entry->sfx_name != "0") {
            sfx_name = emote_entry->sfx_name;
            sfx_looping = emote_entry->sfx_looping;
        }
    }

    if (!sfx_name.empty() && sfx_name != "0" && sfx_name != "1") {
        auto sfx_asset = ao_assets.sound_effect(sfx_name);
        if (sfx_asset && active) {
            EventManager::instance().get_channel<PlaySFXEvent>().publish(PlaySFXEvent(sfx_asset, sfx_looping, 1.0f));
        }
    }

    // Blip player
    ic.blip_player.start(ao_assets, msg.character);
    ic.prev_chars_visible = 0;

    // IC log
    EventManager::instance().get_channel<ICLogEvent>().publish(ICLogEvent(showname, msg.message, msg.text_color));

    return result;
}
