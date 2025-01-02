#include "PacketTypes.h"

#include "AOClient.h"

#include <format>

// decryptor

AOPacketDecryptor::AOPacketDecryptor(const std::vector<std::string>& fields) {
    if (fields.size() >= MIN_FIELDS) {
        header = "decyptor";

        decryptor = fields.at(0);

        this->fields = fields;
        valid = true;
    }
    else {
        valid = false;
        throw PacketFormatException("Not enough fields on packet decryptor");
    }
}

PacketRegistrar AOPacketDecryptor::registrar("decryptor",
                                             [](const std::vector<std::string>& fields) -> std::unique_ptr<AOPacket> {
                                                 return std::make_unique<AOPacketDecryptor>(fields);
                                             });

void AOPacketDecryptor::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received decryptor when client is not in CONNECTED state");
    }

    cli.decryptor = decryptor;

    AOPacketHI hi("bullshit hdid changeme");
    cli.add_message(hi);
}

// HI

AOPacketHI::AOPacketHI(const std::string& hardware_id) : hardware_id(hardware_id) {
    header = "HI";
    fields.push_back(hardware_id);
    valid = true;
}

AOPacketHI::AOPacketHI(const std::vector<std::string>& fields) {
    if (fields.size() >= MIN_FIELDS) {
        header = "HI";

        hardware_id = fields.at(0);

        this->fields = fields;
        valid = true;
    }
    else {
        valid = false;
        throw PacketFormatException("Not enough fields on packet HI");
    }
}

PacketRegistrar AOPacketHI::registrar("HI", [](const std::vector<std::string>& fields) -> std::unique_ptr<AOPacket> {
    return std::make_unique<AOPacketHI>(fields);
});

// ID (version client gets from server)

AOPacketIDClient::AOPacketIDClient(const std::vector<std::string>& fields) {
    if (fields.size() >= MIN_FIELDS) {
        header = "ID";

        player_number = std::atoi(fields.at(0).c_str());
        server_software = fields.at(1);
        server_version = fields.at(2);

        this->fields = fields;
        valid = true;
    }
    else {
        valid = false;
        throw PacketFormatException("Not enough fields on packet ID");
    }
}

PacketRegistrar AOPacketIDClient::registrar("ID",
                                            [](const std::vector<std::string>& fields) -> std::unique_ptr<AOPacket> {
                                                return std::make_unique<AOPacketIDClient>(fields);
                                            });

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

// ID (version client sends to the server)

AOPacketIDServer::AOPacketIDServer(const std::string& client_software, const std::string& client_version)
    : client_software(client_software), client_version(client_version) {
    header = "ID";
    fields.push_back(client_software);
    fields.push_back(client_version);
    valid = true;
}

AOPacketIDServer::AOPacketIDServer(const std::vector<std::string>& fields) {
    if (fields.size() >= MIN_FIELDS) {
        header = "ID";

        client_software = fields.at(0);
        client_version = fields.at(1);

        this->fields = fields;
        valid = true;
    }
    else {
        valid = false;
        throw PacketFormatException("Not enough fields on packet ID");
    }
}

// PN (Player count)

AOPacketPN::AOPacketPN(const std::vector<std::string>& fields) {
    if (fields.size() >= MIN_FIELDS) {
        header = "PN";

        current_players = std::atoi(fields.at(0).c_str());
        max_players = std::atoi(fields.at(1).c_str());

        if (fields.size() >= MIN_FIELDS + 1) {
            server_description = fields.at(2);
        }
        else {
            server_description = "";
        }

        this->fields = fields;
        valid = true;
    }
    else {
        valid = false;
        throw PacketFormatException("Not enough fields on packet PN");
    }
}

PacketRegistrar AOPacketPN::registrar("PN", [](const std::vector<std::string>& fields) -> std::unique_ptr<AOPacket> {
    return std::make_unique<AOPacketPN>(fields);
});

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

// askchaa

AOPacketAskChaa::AOPacketAskChaa() {
    header = "askchaa";
    valid = true;
}

// ASS (Remote Asset URL)

AOPacketASS::AOPacketASS(const std::vector<std::string>& fields) {
    if (fields.size() >= MIN_FIELDS) {
        header = "ASS";

        asset_url = fields.at(0);
        valid = true;
    }
    else {
        valid = false;
        throw PacketFormatException("Not enough fields on packet ASS");
    }
}

PacketRegistrar AOPacketASS::registrar("ASS", [](const std::vector<std::string>& fields) -> std::unique_ptr<AOPacket> {
    return std::make_unique<AOPacketASS>(fields);
});

void AOPacketASS::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received ASS when client is not in CONNECTED state");
    }

    cli.asset_url = asset_url;
}

// SI (Server Information, aka Resource Counts)
// TODO: askchaa

AOPacketSI::AOPacketSI(const std::vector<std::string>& fields) {
    if (fields.size() >= MIN_FIELDS) {
        header = "SI";

        character_count = std::atoi(fields.at(0).c_str());
        evidence_count = std::atoi(fields.at(1).c_str());
        music_count = std::atoi(fields.at(2).c_str());
        valid = true;
    }
    else {
        valid = false;
        throw PacketFormatException("Not enough fields on packet SI");
    }
}

PacketRegistrar AOPacketSI::registrar("SI", [](const std::vector<std::string>& fields) -> std::unique_ptr<AOPacket> {
    return std::make_unique<AOPacketSI>(fields);
});

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

// RC (Request Charlist)

AOPacketRC::AOPacketRC() {
    header = "RC";
    valid = true;
}

// SC (Send Charlist)

AOPacketSC::AOPacketSC(const std::vector<std::string>& fields) {
    if (fields.size() >= MIN_FIELDS) {
        header = "SC";

        for (const std::string& character : fields) {
            character_list.push_back(character);
        }
        valid = true;
    }
    else {
        valid = false;
        throw PacketFormatException("Not enough fields on packet SC");
    }
}

PacketRegistrar AOPacketSC::registrar("SC", [](const std::vector<std::string>& fields) -> std::unique_ptr<AOPacket> {
    return std::make_unique<AOPacketSC>(fields);
});

void AOPacketSC::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received SC when client is not in CONNECTED state");
    }

    cli.character_list = character_list;

    AOPacketRM ask_for_music;
    cli.add_message(ask_for_music);
}

// RM (Request Music)

AOPacketRM::AOPacketRM() {
    header = "RM";
    valid = true;
}

// SM (Sadism/Masochism)

AOPacketSM::AOPacketSM(const std::vector<std::string>& fields) {
    if (fields.size() >= MIN_FIELDS) {
        header = "SM";

        for (const std::string& character : fields) {
            music_list.push_back(character);
        }
        valid = true;
    }
    else {
        valid = false;
        throw PacketFormatException("Not enough fields on packet SM");
    }
}

PacketRegistrar AOPacketSM::registrar("SM", [](const std::vector<std::string>& fields) -> std::unique_ptr<AOPacket> {
    return std::make_unique<AOPacketSM>(fields);
});

void AOPacketSM::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received SM when client is not in CONNECTED state");
    }

    cli.music_list = music_list;

    AOPacketRD signal_done;
    cli.add_message(signal_done);
}

// RD

AOPacketRD::AOPacketRD() {
    header = "RD";
    valid = true;
}

// DONE

AOPacketDONE::AOPacketDONE() {
    header = "DONE";
    valid = true;
}
PacketRegistrar AOPacketDONE::registrar("DONE",
                                        [](const std::vector<std::string>& fields) -> std::unique_ptr<AOPacket> {
                                            return std::make_unique<AOPacketDONE>();
                                        });

void AOPacketDONE::handle(AOClient& cli) {
    if (cli.conn_state != CONNECTED) {
        throw ProtocolStateException("Received DONE when client is not in CONNECTED state");
    }
    // do nothing for now
}
