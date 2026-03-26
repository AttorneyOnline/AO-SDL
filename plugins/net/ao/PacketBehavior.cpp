#include "PacketTypes.h"

#include "AOClient.h"
#include "ao/event/ICMessageEvent.h"
#include "ao/event/PlayerCountEvent.h"
#include "ao/event/ServerInfoEvent.h"
#include "event/AreaUpdateEvent.h"
#include "event/AssetUrlEvent.h"
#include "event/BackgroundEvent.h"
#include "event/CharacterListEvent.h"
#include "event/CharsCheckEvent.h"
#include "event/ChatEvent.h"
#include "event/EventManager.h"
#include "event/EvidenceListEvent.h"
#include "event/FeatureListEvent.h"
#include "event/HealthBarEvent.h"
#include "event/MusicChangeEvent.h"
#include "event/MusicListEvent.h"
#include "event/PlayerListEvent.h"
#include "event/TimerEvent.h"
#include "event/UIEvent.h"
#include "platform/HardwareId.h"
#include "utils/Version.h"

// Keeping the actual handler functions in a separate file here just for clarity

void AOPacketDecryptor::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received decryptor when client is not in CONNECTED state");
    }

    cli.decryptor = decryptor;

    AOPacketHI hi(platform::hardware_id());
    cli.add_message(hi);
}

void AOPacketIDClient::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received ID when client is not in CONNECTED state");
    }

    cli.player_number = player_number;

    EventManager::instance().get_channel<ServerInfoEvent>().publish(ServerInfoEvent(software, version, player_number));

    AOPacketIDServer id_to_server(std::string("AO-SDL/") + ao_sdl_version(), "2.999.999");
    cli.add_message(id_to_server);
}

void AOPacketPN::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received PN when client is not in CONNECTED state");
    }

    EventManager::instance().get_channel<PlayerCountEvent>().publish(
        PlayerCountEvent(current_players, max_players, server_description));

    AOPacketAskChaa ask_chars;
    cli.add_message(ask_chars);
}

void AOPacketASS::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received ASS when client is not in CONNECTED state");
    }

    cli.asset_url = asset_url;
    if (!asset_url.empty()) {
        EventManager::instance().get_channel<AssetUrlEvent>().publish(AssetUrlEvent(asset_url));
    }
}

void AOPacketSI::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received SI when client is not in CONNECTED state");
    }

    cli.character_count = character_count;
    cli.evidence_count = evidence_count;
    cli.music_count = music_count;

    AOPacketRC ask_for_chars;
    cli.add_message(ask_for_chars);
}

void AOPacketSC::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received SC when client is not in CONNECTED state");
    }

    cli.character_list = character_list;

    // Each field is "folder_name&display_name" — extract just the folder name.
    std::vector<std::string> folder_names;
    folder_names.reserve(character_list.size());
    for (const auto& entry : character_list) {
        folder_names.push_back(entry.substr(0, entry.find('&')));
    }

    EventManager::instance().get_channel<CharacterListEvent>().publish(CharacterListEvent(std::move(folder_names)));

    AOPacketRM ask_for_music;
    cli.add_message(ask_for_music);
}

static bool has_audio_extension(const std::string& name) {
    static const std::string exts[] = {".wav", ".mp3", ".mp4", ".ogg", ".opus"};
    for (const auto& ext : exts) {
        if (name.size() >= ext.size() && name.compare(name.size() - ext.size(), ext.size(), ext) == 0)
            return true;
    }
    return false;
}

void AOPacketSM::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received SM when client is not in CONNECTED state");
    }

    // Split the combined list: areas first, then music tracks.
    // The boundary is the first entry with an audio file extension.
    std::vector<std::string> areas;
    std::vector<std::string> tracks;
    bool in_music = false;

    for (const auto& entry : music_list) {
        if (in_music) {
            tracks.push_back(entry);
        }
        else if (has_audio_extension(entry)) {
            // The previous "area" was actually a category header for music
            if (!areas.empty()) {
                tracks.push_back(areas.back());
                areas.pop_back();
            }
            tracks.push_back(entry);
            in_music = true;
        }
        else {
            areas.push_back(entry);
        }
    }

    EventManager::instance().get_channel<MusicListEvent>().publish(MusicListEvent(std::move(areas), std::move(tracks)));

    AOPacketRD signal_done;
    cli.add_message(signal_done);
}

void AOPacketCHECK::handle(AOClient& cli) {
    // Server acknowledged our keepalive. Could measure latency here.
}

void AOPacketDONE::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received DONE when client is not in CONNECTED state");
    }

    cli.conn_state = JOINED;

    EventManager::instance().get_channel<UIEvent>().publish(UIEvent(UIEventType::CHAR_LOADING_DONE));
    // do nothing else for now
}

void AOPacketCharsCheck::handle(AOClient& cli) {
    EventManager::instance().get_channel<CharsCheckEvent>().publish(CharsCheckEvent(taken));
}

void AOPacketMS::handle(AOClient& cli) {
    EventManager::instance().get_channel<ICMessageEvent>().publish(ICMessageEvent(
        character, emote, pre_emote, message, showname, side, static_cast<EmoteMod>(emote_mod),
        static_cast<DeskMod>(desk_mod), flip, char_id, text_color, objection_mod, screenshake, realization, additive,
        frame_screenshake, sfx_name, sfx_delay, sfx_looping, frame_sfx, immediate, slide));
}

void AOPacketBN::handle(AOClient& cli) {
    EventManager::instance().get_channel<BackgroundEvent>().publish(BackgroundEvent(background, position));
}

void AOPacketMC::handle(AOClient& cli) {
    EventManager::instance().get_channel<MusicChangeEvent>().publish(
        MusicChangeEvent(name, char_id, showname, looping, channel, effect_flags));
}

void AOPacketARUP::handle(AOClient& cli) {
    if (arup_type < 0 || arup_type > 3)
        return;
    EventManager::instance().get_channel<AreaUpdateEvent>().publish(
        AreaUpdateEvent(static_cast<AreaUpdateEvent::Type>(arup_type), values));
}

void AOPacketPV::handle(AOClient& cli) {
    cli.char_id = char_id;
    std::string character_name;
    if (char_id >= 0 && char_id < (int)cli.character_list.size())
        character_name = cli.character_list[char_id];
    EventManager::instance().get_channel<UIEvent>().publish(
        UIEvent(UIEventType::ENTERED_COURTROOM, character_name, char_id));
}

void AOPacketCT::handle(AOClient& cli) {
    EventManager::instance().get_channel<ChatEvent>().publish(ChatEvent(sender_name, message, system_message));
}

void AOPacketFL::handle(AOClient& cli) {
    cli.features = features;
    EventManager::instance().get_channel<FeatureListEvent>().publish(FeatureListEvent(features));
}

void AOPacketFA::handle(AOClient& cli) {
    EventManager::instance().get_channel<MusicListEvent>().publish(MusicListEvent(areas, {}, true));
}

void AOPacketFM::handle(AOClient& cli) {
    EventManager::instance().get_channel<MusicListEvent>().publish(MusicListEvent({}, tracks, true));
}

void AOPacketHP::handle(AOClient& cli) {
    EventManager::instance().get_channel<HealthBarEvent>().publish(HealthBarEvent(side, value));
}

void AOPacketTI::handle(AOClient& cli) {
    EventManager::instance().get_channel<TimerEvent>().publish(TimerEvent(timer_id, action, time_ms));
}

void AOPacketLE::handle(AOClient& cli) {
    std::vector<EvidenceItem> items;
    for (const auto& raw : raw_items) {
        EvidenceItem item;
        size_t p1 = raw.find('&');
        if (p1 == std::string::npos) {
            item.name = raw;
        }
        else {
            item.name = raw.substr(0, p1);
            size_t p2 = raw.find('&', p1 + 1);
            if (p2 == std::string::npos) {
                item.description = raw.substr(p1 + 1);
            }
            else {
                item.description = raw.substr(p1 + 1, p2 - p1 - 1);
                item.image = raw.substr(p2 + 1);
            }
        }
        items.push_back(std::move(item));
    }
    EventManager::instance().get_channel<EvidenceListEvent>().publish(EvidenceListEvent(std::move(items)));
}

void AOPacketPR::handle(AOClient& cli) {
    auto action = (update_type == 0) ? PlayerListEvent::Action::ADD : PlayerListEvent::Action::REMOVE;
    EventManager::instance().get_channel<PlayerListEvent>().publish(PlayerListEvent(action, player_id));
}

void AOPacketPU::handle(AOClient& cli) {
    PlayerListEvent::Action action;
    switch (data_type) {
    case 0:
        action = PlayerListEvent::Action::UPDATE_NAME;
        break;
    case 1:
        action = PlayerListEvent::Action::UPDATE_CHARACTER;
        break;
    case 2:
        action = PlayerListEvent::Action::UPDATE_CHARNAME;
        break;
    case 3:
        action = PlayerListEvent::Action::UPDATE_AREA;
        break;
    default:
        return;
    }
    EventManager::instance().get_channel<PlayerListEvent>().publish(PlayerListEvent(action, player_id, data));
}
