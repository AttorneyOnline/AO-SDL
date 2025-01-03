#include "PacketTypes.h"

#include "AOClient.h"
#include "event/EventManager.h"

// Keeping the actual handler functions in a separate file here just for clarity

void AOPacketDecryptor::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received decryptor when client is not in CONNECTED state");
    }

    cli.decryptor = decryptor;

    AOPacketHI hi("bullshit hdid changeme");
    cli.add_message(hi);
}

void AOPacketIDClient::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received ID when client is not in CONNECTED state");
    }

    cli.player_number = player_number;
    cli.server_software = server_software;
    cli.server_version = server_version;

    AOPacketIDServer id_to_server("tsurushiage", "2.999.999");
    cli.add_message(id_to_server);
}

void AOPacketPN::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received PN when client is not in CONNECTED state");
    }

    cli.current_players = current_players;
    cli.max_players = max_players;
    cli.server_description = server_description;

    AOPacketAskChaa ask_chars;
    cli.add_message(ask_chars);
}

void AOPacketASS::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received ASS when client is not in CONNECTED state");
    }

    cli.asset_url = asset_url;
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

    AOPacketRM ask_for_music;
    cli.add_message(ask_for_music);
}

void AOPacketSM::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received SM when client is not in CONNECTED state");
    }

    cli.music_list = music_list;

    AOPacketRD signal_done;
    cli.add_message(signal_done);
}

void AOPacketDONE::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received DONE when client is not in CONNECTED state");
    }

    cli.conn_state = JOINED;

    EventManager::get_instance().get_ui_channel()->publish(UIEvent(UIEventType::CHAR_LOADING_DONE));
    // do nothing else for now
}

void AOPacketCT::handle(AOClient& cli) {
    EventManager::get_instance().get_chat_channel()->publish(ChatEvent(sender_name, message, system_message));
}