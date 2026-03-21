#include "PacketTypes.h"

#include <algorithm>

// AO protocol encoding: special characters must be escaped in field values.
static std::string ao_encode(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '#':
            out += "<pound>";
            break;
        case '%':
            out += "<percent>";
            break;
        case '&':
            out += "<and>";
            break;
        case '$':
            out += "<dollar>";
            break;
        default:
            out += c;
        }
    }
    return out;
}

static std::string ao_decode(const std::string& s) {
    std::string out = s;
    auto replace_all = [&](const std::string& from, const std::string& to) {
        size_t pos = 0;
        while ((pos = out.find(from, pos)) != std::string::npos) {
            out.replace(pos, from.size(), to);
            pos += to.size();
        }
    };
    replace_all("<pound>", "#");
    replace_all("<percent>", "%");
    replace_all("<and>", "&");
    replace_all("<dollar>", "$");
    return out;
}

// ---------------------------------------------------------------------------
// AOPacketDecryptor
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketDecryptor::registrar("decryptor",
                                             [](const auto& f) { return std::make_unique<AOPacketDecryptor>(f); });

AOPacketDecryptor::AOPacketDecryptor(const std::vector<std::string>& fields) : AOPacket("decryptor", fields) {
    if (fields.size() >= MIN_FIELDS) {
        decryptor = fields[0];
    }
}

// ---------------------------------------------------------------------------
// AOPacketHI
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketHI::registrar("HI", [](const auto& f) { return std::make_unique<AOPacketHI>(f); });

AOPacketHI::AOPacketHI(const std::string& hardware_id) : AOPacket("HI", {hardware_id}), hardware_id(hardware_id) {
}

AOPacketHI::AOPacketHI(const std::vector<std::string>& fields) : AOPacket("HI", fields) {
    if (fields.size() >= MIN_FIELDS) {
        hardware_id = fields[0];
    }
}

// ---------------------------------------------------------------------------
// AOPacketIDClient  (server → client)
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketIDClient::registrar("ID", [](const auto& f) { return std::make_unique<AOPacketIDClient>(f); });

AOPacketIDClient::AOPacketIDClient(const std::vector<std::string>& fields) : AOPacket("ID", fields) {
    if (fields.size() >= MIN_FIELDS) {
        player_number = std::stoi(fields[0]);
        server_software = fields[1];
        server_version = fields[2];
    }
}

// ---------------------------------------------------------------------------
// AOPacketIDServer  (client → server)
// ---------------------------------------------------------------------------

AOPacketIDServer::AOPacketIDServer(const std::string& client_software, const std::string& client_version)
    : AOPacket("ID", {client_software, client_version}), client_software(client_software),
      client_version(client_version) {
}

AOPacketIDServer::AOPacketIDServer(const std::vector<std::string>& fields) : AOPacket("ID", fields) {
    if (fields.size() >= MIN_FIELDS) {
        client_software = fields[0];
        client_version = fields[1];
    }
}

// ---------------------------------------------------------------------------
// AOPacketPN
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketPN::registrar("PN", [](const auto& f) { return std::make_unique<AOPacketPN>(f); });

AOPacketPN::AOPacketPN(const std::vector<std::string>& fields) : AOPacket("PN", fields) {
    if (fields.size() >= MIN_FIELDS) {
        current_players = std::stoi(fields[0]);
        max_players = std::stoi(fields[1]);
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

PacketRegistrar AOPacketASS::registrar("ASS", [](const auto& f) { return std::make_unique<AOPacketASS>(f); });

AOPacketASS::AOPacketASS(const std::vector<std::string>& fields) : AOPacket("ASS", fields) {
    if (fields.size() >= MIN_FIELDS) {
        asset_url = fields[0];
    }
}

// ---------------------------------------------------------------------------
// AOPacketSI
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketSI::registrar("SI", [](const auto& f) { return std::make_unique<AOPacketSI>(f); });

AOPacketSI::AOPacketSI(const std::vector<std::string>& fields) : AOPacket("SI", fields) {
    if (fields.size() >= MIN_FIELDS) {
        character_count = std::stoi(fields[0]);
        evidence_count = std::stoi(fields[1]);
        music_count = std::stoi(fields[2]);
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

PacketRegistrar AOPacketSC::registrar("SC", [](const auto& f) { return std::make_unique<AOPacketSC>(f); });

AOPacketSC::AOPacketSC(const std::vector<std::string>& fields) : AOPacket("SC", fields), character_list(fields) {
}

// ---------------------------------------------------------------------------
// AOPacketRM  (client → server, no handle)
// ---------------------------------------------------------------------------

AOPacketRM::AOPacketRM() : AOPacket("RM", {}) {
}

// ---------------------------------------------------------------------------
// AOPacketSM
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketSM::registrar("SM", [](const auto& f) { return std::make_unique<AOPacketSM>(f); });

AOPacketSM::AOPacketSM(const std::vector<std::string>& fields) : AOPacket("SM", fields), music_list(fields) {
}

// ---------------------------------------------------------------------------
// AOPacketCH  (client → server keepalive, no handle)
// ---------------------------------------------------------------------------

AOPacketCH::AOPacketCH(int char_id) : AOPacket("CH", {std::to_string(char_id)}) {
}

// ---------------------------------------------------------------------------
// AOPacketCHECK  (server → client keepalive response)
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketCHECK::registrar("CHECK", [](const auto& f) { return std::make_unique<AOPacketCHECK>(f); });

AOPacketCHECK::AOPacketCHECK(const std::vector<std::string>& fields) : AOPacket("CHECK", fields) {
}

// ---------------------------------------------------------------------------
// AOPacketRD  (client → server, no handle)
// ---------------------------------------------------------------------------

AOPacketRD::AOPacketRD() : AOPacket("RD", {}) {
}

// ---------------------------------------------------------------------------
// AOPacketDONE
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketDONE::registrar("DONE", [](const auto& f) { return std::make_unique<AOPacketDONE>(); });

AOPacketDONE::AOPacketDONE() : AOPacket("DONE", {}) {
}

// ---------------------------------------------------------------------------
// AOPacketCharsCheck
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketCharsCheck::registrar("CharsCheck",
                                              [](const auto& f) { return std::make_unique<AOPacketCharsCheck>(f); });

AOPacketCharsCheck::AOPacketCharsCheck(const std::vector<std::string>& fields) : AOPacket("CharsCheck", fields) {
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

#include "ao/event/OutgoingICMessageEvent.h"

PacketRegistrar AOPacketMS::registrar("MS", [](const auto& f) { return std::make_unique<AOPacketMS>(f); });

AOPacketMS::AOPacketMS(const ICMessageData& d)
    : AOPacket("MS",
               {// 0-14: core fields
                std::to_string(d.desk_mod), ao_encode(d.pre_emote), ao_encode(d.character), ao_encode(d.emote),
                ao_encode(d.message), ao_encode(d.side), ao_encode(d.sfx_name), std::to_string(d.emote_mod),
                std::to_string(d.char_id), std::to_string(d.sfx_delay), std::to_string(d.objection_mod),
                std::to_string(d.evidence_id), std::to_string(d.flip), std::to_string(d.realization),
                std::to_string(d.text_color),
                // 15-18: 2.6 extensions (client→server format)
                ao_encode(d.showname), std::to_string(d.other_charid),
                ao_encode(d.self_offset), std::to_string(d.immediate),
                // 19-25: 2.8 extensions
                std::to_string(d.looping_sfx), std::to_string(d.screenshake), ao_encode(d.frame_screenshake),
                ao_encode(d.frame_realization), ao_encode(d.frame_sfx), std::to_string(d.additive),
                ao_encode(d.effects)}) {
}

AOPacketMS::AOPacketMS(const std::vector<std::string>& fields) : AOPacket("MS", fields) {
    if (fields.size() >= MIN_FIELDS) {
        desk_mod = std::stoi(fields[0]);
        pre_emote = ao_decode(fields[1]);
        character = ao_decode(fields[2]);
        emote = ao_decode(fields[3]);
        message = ao_decode(fields[4]);
        side = ao_decode(fields[5]);
        // fields[6] = sfx_name (skipped for now)
        emote_mod = std::stoi(fields[7]);
        char_id = std::stoi(fields[8]);
        // fields[9] = sfx_delay (skipped)
        objection_mod = std::stoi(fields[10]);
        // fields[11] = evidence_id (skipped)
        flip = fields[12] == "1";
        realization = fields[13] == "1";
        text_color = std::stoi(fields[14]);

        // Showname (optional field 15)
        showname = fields.size() > 15 ? ao_decode(fields[15]) : character;

        // 2.8 extensions (server→client echo format)
        // 23: looping_sfx, 24: screenshake, 25: frame_screenshake,
        // 26: frame_realization, 27: frame_sfx, 28: additive, 29: effects
        if (fields.size() > 24)
            screenshake = fields[24] == "1";
        if (fields.size() > 25)
            frame_screenshake = ao_decode(fields[25]);
        if (fields.size() > 28)
            additive = fields[28] == "1";

        // Legacy emote_mod remapping
        if (emote_mod == 4)
            emote_mod = 6; // legacy → PREANIM_ZOOM
        if (emote_mod == 2)
            emote_mod = 1; // deprecated → PREANIM
        if (emote_mod != 0 && emote_mod != 1 && emote_mod != 5 && emote_mod != 6) {
            emote_mod = 0; // invalid → IDLE
        }
    }
}

// ---------------------------------------------------------------------------
// AOPacketMC
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketMC::registrar("MC", [](const auto& f) { return std::make_unique<AOPacketMC>(f); });

AOPacketMC::AOPacketMC(const std::string& name, int char_id, const std::string& showname)
    : AOPacket("MC", showname.empty()
                         ? std::vector<std::string>{ao_encode(name), std::to_string(char_id)}
                         : std::vector<std::string>{ao_encode(name), std::to_string(char_id), ao_encode(showname)}),
      name(name), char_id(char_id), showname(showname) {
}

AOPacketMC::AOPacketMC(const std::vector<std::string>& fields) : AOPacket("MC", fields) {
    if (fields.size() >= MIN_FIELDS) {
        name = ao_decode(fields[0]);
        char_id = std::stoi(fields[1]);
        if (fields.size() > 2)
            showname = ao_decode(fields[2]);
    }
}

// ---------------------------------------------------------------------------
// AOPacketARUP
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketARUP::registrar("ARUP", [](const auto& f) { return std::make_unique<AOPacketARUP>(f); });

AOPacketARUP::AOPacketARUP(const std::vector<std::string>& fields) : AOPacket("ARUP", fields) {
    if (fields.size() >= MIN_FIELDS) {
        arup_type = std::stoi(fields[0]);
        values.assign(fields.begin() + 1, fields.end());
    }
}

// ---------------------------------------------------------------------------
// AOPacketBN
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketBN::registrar("BN", [](const auto& f) { return std::make_unique<AOPacketBN>(f); });

AOPacketBN::AOPacketBN(const std::vector<std::string>& fields) : AOPacket("BN", fields) {
    if (fields.size() >= MIN_FIELDS) {
        background = fields[0];
        position = fields.size() >= 2 ? fields[1] : "";
    }
}

// ---------------------------------------------------------------------------
// AOPacketPV
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketPV::registrar("PV", [](const auto& f) { return std::make_unique<AOPacketPV>(f); });

AOPacketPV::AOPacketPV(const std::vector<std::string>& fields) : AOPacket("PV", fields) {
    if (fields.size() >= MIN_FIELDS) {
        char_id = std::stoi(fields[2]);
    }
}

// ---------------------------------------------------------------------------
// AOPacketFL
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketFL::registrar("FL", [](const auto& f) { return std::make_unique<AOPacketFL>(f); });

AOPacketFL::AOPacketFL(const std::vector<std::string>& fields) : AOPacket("FL", fields), features(fields) {
}

// ---------------------------------------------------------------------------
// AOPacketFA
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketFA::registrar("FA", [](const auto& f) { return std::make_unique<AOPacketFA>(f); });

AOPacketFA::AOPacketFA(const std::vector<std::string>& fields) : AOPacket("FA", fields), areas(fields) {
}

// ---------------------------------------------------------------------------
// AOPacketFM
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketFM::registrar("FM", [](const auto& f) { return std::make_unique<AOPacketFM>(f); });

AOPacketFM::AOPacketFM(const std::vector<std::string>& fields) : AOPacket("FM", fields), tracks(fields) {
}

// ---------------------------------------------------------------------------
// AOPacketHP
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketHP::registrar("HP", [](const auto& f) { return std::make_unique<AOPacketHP>(f); });

AOPacketHP::AOPacketHP(const std::vector<std::string>& fields) : AOPacket("HP", fields) {
    if (fields.size() >= MIN_FIELDS) {
        side = std::stoi(fields[0]);
        value = std::stoi(fields[1]);
    }
}

// ---------------------------------------------------------------------------
// AOPacketTI
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketTI::registrar("TI", [](const auto& f) { return std::make_unique<AOPacketTI>(f); });

AOPacketTI::AOPacketTI(const std::vector<std::string>& fields) : AOPacket("TI", fields) {
    if (fields.size() >= MIN_FIELDS) {
        timer_id = std::stoi(fields[0]);
        action = std::stoi(fields[1]);
        if (fields.size() > 2)
            time_ms = std::stoll(fields[2]);
    }
}

// ---------------------------------------------------------------------------
// AOPacketLE
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketLE::registrar("LE", [](const auto& f) { return std::make_unique<AOPacketLE>(f); });

AOPacketLE::AOPacketLE(const std::vector<std::string>& fields) : AOPacket("LE", fields), raw_items(fields) {
}

// ---------------------------------------------------------------------------
// AOPacketPR
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketPR::registrar("PR", [](const auto& f) { return std::make_unique<AOPacketPR>(f); });

AOPacketPR::AOPacketPR(const std::vector<std::string>& fields) : AOPacket("PR", fields) {
    if (fields.size() >= MIN_FIELDS) {
        player_id = std::stoi(fields[0]);
        update_type = std::stoi(fields[1]);
    }
}

// ---------------------------------------------------------------------------
// AOPacketPU
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketPU::registrar("PU", [](const auto& f) { return std::make_unique<AOPacketPU>(f); });

AOPacketPU::AOPacketPU(const std::vector<std::string>& fields) : AOPacket("PU", fields) {
    if (fields.size() >= MIN_FIELDS) {
        player_id = std::stoi(fields[0]);
        data_type = std::stoi(fields[1]);
        data = fields[2];
    }
}

// ---------------------------------------------------------------------------
// AOPacketCT
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketCT::registrar("CT", [](const auto& f) { return std::make_unique<AOPacketCT>(f); });

AOPacketCT::AOPacketCT(const std::string& sender_name, const std::string& message, bool system_message)
    : AOPacket("CT", {ao_encode(sender_name), ao_encode(message), system_message ? "1" : "0"}),
      sender_name(sender_name), message(message), system_message(system_message) {
}

AOPacketCT::AOPacketCT(const std::vector<std::string>& fields) : AOPacket("CT", fields) {
    if (fields.size() >= MIN_FIELDS) {
        sender_name = ao_decode(fields[0]);
        message = ao_decode(fields[1]);
        system_message = fields.size() > 2 && fields[2] == "1";
    }
}
