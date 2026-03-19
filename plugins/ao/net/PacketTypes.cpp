#include "PacketTypes.h"

// ---------------------------------------------------------------------------
// AOPacketDecryptor
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketDecryptor::registrar("decryptor", [](const auto& f) {
    return std::make_unique<AOPacketDecryptor>(f);
});

AOPacketDecryptor::AOPacketDecryptor(const std::vector<std::string>& fields)
    : AOPacket("decryptor", fields) {
    if (fields.size() >= MIN_FIELDS) {
        decryptor = fields[0];
    }
}

// ---------------------------------------------------------------------------
// AOPacketHI
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketHI::registrar("HI", [](const auto& f) {
    return std::make_unique<AOPacketHI>(f);
});

AOPacketHI::AOPacketHI(const std::string& hardware_id)
    : AOPacket("HI", {hardware_id}), hardware_id(hardware_id) {
}

AOPacketHI::AOPacketHI(const std::vector<std::string>& fields)
    : AOPacket("HI", fields) {
    if (fields.size() >= MIN_FIELDS) {
        hardware_id = fields[0];
    }
}

// ---------------------------------------------------------------------------
// AOPacketIDClient  (server → client)
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketIDClient::registrar("ID", [](const auto& f) {
    return std::make_unique<AOPacketIDClient>(f);
});

AOPacketIDClient::AOPacketIDClient(const std::vector<std::string>& fields)
    : AOPacket("ID", fields) {
    if (fields.size() >= MIN_FIELDS) {
        player_number  = std::stoi(fields[0]);
        server_software = fields[1];
        server_version  = fields[2];
    }
}

// ---------------------------------------------------------------------------
// AOPacketIDServer  (client → server)
// ---------------------------------------------------------------------------

AOPacketIDServer::AOPacketIDServer(const std::string& client_software, const std::string& client_version)
    : AOPacket("ID", {client_software, client_version}),
      client_software(client_software), client_version(client_version) {
}

AOPacketIDServer::AOPacketIDServer(const std::vector<std::string>& fields)
    : AOPacket("ID", fields) {
    if (fields.size() >= MIN_FIELDS) {
        client_software = fields[0];
        client_version  = fields[1];
    }
}

// ---------------------------------------------------------------------------
// AOPacketPN
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketPN::registrar("PN", [](const auto& f) {
    return std::make_unique<AOPacketPN>(f);
});

AOPacketPN::AOPacketPN(const std::vector<std::string>& fields)
    : AOPacket("PN", fields) {
    if (fields.size() >= MIN_FIELDS) {
        current_players    = std::stoi(fields[0]);
        max_players        = std::stoi(fields[1]);
        server_description = fields.size() > 2 ? fields[2] : "";
    }
}

// ---------------------------------------------------------------------------
// AOPacketAskChaa  (client → server, no handle)
// ---------------------------------------------------------------------------

AOPacketAskChaa::AOPacketAskChaa() : AOPacket("askchaa", {}) {
}

// ---------------------------------------------------------------------------
// AOPacketASS
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketASS::registrar("ASS", [](const auto& f) {
    return std::make_unique<AOPacketASS>(f);
});

AOPacketASS::AOPacketASS(const std::vector<std::string>& fields)
    : AOPacket("ASS", fields) {
    if (fields.size() >= MIN_FIELDS) {
        asset_url = fields[0];
    }
}

// ---------------------------------------------------------------------------
// AOPacketSI
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketSI::registrar("SI", [](const auto& f) {
    return std::make_unique<AOPacketSI>(f);
});

AOPacketSI::AOPacketSI(const std::vector<std::string>& fields)
    : AOPacket("SI", fields) {
    if (fields.size() >= MIN_FIELDS) {
        character_count = std::stoi(fields[0]);
        evidence_count  = std::stoi(fields[1]);
        music_count     = std::stoi(fields[2]);
    }
}

// ---------------------------------------------------------------------------
// AOPacketRC  (client → server, no handle)
// ---------------------------------------------------------------------------

AOPacketRC::AOPacketRC() : AOPacket("RC", {}) {
}

// ---------------------------------------------------------------------------
// AOPacketSC
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketSC::registrar("SC", [](const auto& f) {
    return std::make_unique<AOPacketSC>(f);
});

AOPacketSC::AOPacketSC(const std::vector<std::string>& fields)
    : AOPacket("SC", fields), character_list(fields) {
}

// ---------------------------------------------------------------------------
// AOPacketRM  (client → server, no handle)
// ---------------------------------------------------------------------------

AOPacketRM::AOPacketRM() : AOPacket("RM", {}) {
}

// ---------------------------------------------------------------------------
// AOPacketSM
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketSM::registrar("SM", [](const auto& f) {
    return std::make_unique<AOPacketSM>(f);
});

AOPacketSM::AOPacketSM(const std::vector<std::string>& fields)
    : AOPacket("SM", fields), music_list(fields) {
}

// ---------------------------------------------------------------------------
// AOPacketRD  (client → server, no handle)
// ---------------------------------------------------------------------------

AOPacketRD::AOPacketRD() : AOPacket("RD", {}) {
}

// ---------------------------------------------------------------------------
// AOPacketDONE
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketDONE::registrar("DONE", [](const auto& f) {
    return std::make_unique<AOPacketDONE>();
});

AOPacketDONE::AOPacketDONE() : AOPacket("DONE", {}) {
}

// ---------------------------------------------------------------------------
// AOPacketCharsCheck
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketCharsCheck::registrar("CharsCheck", [](const auto& f) {
    return std::make_unique<AOPacketCharsCheck>(f);
});

AOPacketCharsCheck::AOPacketCharsCheck(const std::vector<std::string>& fields)
    : AOPacket("CharsCheck", fields) {
    taken.reserve(fields.size());
    for (const auto& f : fields) {
        taken.push_back(f == "-1");
    }
}

// ---------------------------------------------------------------------------
// AOPacketPW
// ---------------------------------------------------------------------------

AOPacketPW::AOPacketPW(const std::string& password) : AOPacket("PW", {password}) {
}

// ---------------------------------------------------------------------------
// AOPacketCC
// ---------------------------------------------------------------------------

AOPacketCC::AOPacketCC(int player_num, int char_id, const std::string& hdid)
    : AOPacket("CC", {std::to_string(player_num), std::to_string(char_id), hdid}) {
}

// ---------------------------------------------------------------------------
// AOPacketMS
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketMS::registrar("MS", [](const auto& f) {
    return std::make_unique<AOPacketMS>(f);
});

AOPacketMS::AOPacketMS(const std::vector<std::string>& fields)
    : AOPacket("MS", fields) {
    if (fields.size() >= MIN_FIELDS) {
        desk_mod   = std::stoi(fields[0]);
        pre_emote  = fields[1];
        character  = fields[2];
        emote      = fields[3];
        message    = fields[4];
        side       = fields[5];
        // fields[6] = sfx_name (skipped for now)
        emote_mod  = std::stoi(fields[7]);
        char_id    = std::stoi(fields[8]);
        // fields[9] = sfx_delay (skipped)
        // fields[10] = objection_mod (skipped)
        // fields[11] = evidence_id (skipped)
        flip       = fields[12] == "1";
        // fields[13] = realization (skipped)
        text_color = std::stoi(fields[14]);

        // Showname (optional field 15)
        showname   = fields.size() > 15 ? fields[15] : character;

        // Legacy emote_mod remapping
        if (emote_mod == 4) emote_mod = 6; // legacy → PREANIM_ZOOM
        if (emote_mod == 2) emote_mod = 1; // deprecated → PREANIM
        if (emote_mod != 0 && emote_mod != 1 && emote_mod != 5 && emote_mod != 6) {
            emote_mod = 0; // invalid → IDLE
        }
    }
}

// ---------------------------------------------------------------------------
// AOPacketBN
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketBN::registrar("BN", [](const auto& f) {
    return std::make_unique<AOPacketBN>(f);
});

AOPacketBN::AOPacketBN(const std::vector<std::string>& fields)
    : AOPacket("BN", fields) {
    if (fields.size() >= MIN_FIELDS) {
        background = fields[0];
        position = fields.size() >= 2 ? fields[1] : "";
    }
}

// ---------------------------------------------------------------------------
// AOPacketPV
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketPV::registrar("PV", [](const auto& f) {
    return std::make_unique<AOPacketPV>(f);
});

AOPacketPV::AOPacketPV(const std::vector<std::string>& fields)
    : AOPacket("PV", fields) {
    if (fields.size() >= MIN_FIELDS) {
        char_id = std::stoi(fields[2]);
    }
}

// ---------------------------------------------------------------------------
// AOPacketCT
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketCT::registrar("CT", [](const auto& f) {
    return std::make_unique<AOPacketCT>(f);
});

AOPacketCT::AOPacketCT(const std::string& sender_name, const std::string& message, bool system_message)
    : AOPacket("CT", {sender_name, message, system_message ? "1" : "0"}),
      sender_name(sender_name), message(message), system_message(system_message) {
}

AOPacketCT::AOPacketCT(const std::vector<std::string>& fields)
    : AOPacket("CT", fields) {
    if (fields.size() >= MIN_FIELDS) {
        sender_name    = fields[0];
        message        = fields[1];
        system_message = fields.size() > 2 && fields[2] == "1";
    }
}
