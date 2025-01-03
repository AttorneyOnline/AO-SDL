#include "PacketTypes.h"

#include <format>

// This file only includes the logic to construct packets and do validation
// Actual behavior handling is in PacketBehavior.cpp

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

// CT

AOPacketCT::AOPacketCT(const std::string& sender_name, const std::string& message, bool system_message)
    : sender_name(sender_name), message(message), system_message(system_message) {
    header = "CT";

    fields.push_back(sender_name);
    fields.push_back(message);
    if (system_message) {
        fields.push_back("1");
    }

    valid = true;
}

AOPacketCT::AOPacketCT(const std::vector<std::string>& fields) {
    if (fields.size() >= MIN_FIELDS) {
        header = "CT";

        sender_name = fields.at(0);
        message = fields.at(1);

        if (fields.size() >= MIN_FIELDS + 1) {
            system_message = std::atoi(fields.at(2).c_str()) == 1;
        }
        else {
            system_message = false;
        }

        this->fields = fields;
        valid = true;
    }
    else {
        valid = false;
        throw PacketFormatException("Not enough fields on packet CT");
    }
}

PacketRegistrar AOPacketCT::registrar("CT", [](const std::vector<std::string>& fields) -> std::unique_ptr<AOPacket> {
    return std::make_unique<AOPacketCT>(fields);
});