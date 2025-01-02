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

        this->fields = fields;
        valid = true;
    }
    else {
        valid = false;
        throw PacketFormatException("Not enough fields on packet PN");
    }
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

// RC (Request Charlist)

AOPacketRC::AOPacketRC() {
    header = "RC";
    valid = true;
}

// todo: not sure this works; RC has no fields
PacketRegistrar AOPacketRC::registrar("RC", [](const std::vector<std::string>& fields) -> std::unique_ptr<AOPacket> {
    return std::make_unique<AOPacketRC>();
});

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

// RM (Request Music)

AOPacketRM::AOPacketRM() {
    header = "RM";
    valid = true;
}

// todo: not sure this works; RM has no fields
PacketRegistrar AOPacketRM::registrar("RM", [](const std::vector<std::string>& fields) -> std::unique_ptr<AOPacket> {
    return std::make_unique<AOPacketRM>();
});

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

// RM

// SM

// RD

// DONE
