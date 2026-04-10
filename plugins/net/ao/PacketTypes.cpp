#include "PacketTypes.h"

#include "utils/Log.h"

#include <algorithm>
#include <charconv>
#include <string_view>

// ---------------------------------------------------------------------------
// safe_stoi — non-throwing integer parser for wire protocol fields.
//
// `std::stoi` throws `std::invalid_argument` on non-numeric input and
// `std::out_of_range` on overflow, neither of which is appropriate when
// parsing untrusted client data. A single bad numeric field (see the webAO
// `NaN` in the `sfx_delay` slot of MS) would otherwise escape the packet
// constructor, bubble up through `AOPacket::deserialize`, and — if any
// caller forgets to wrap the call in try/catch — terminate a worker thread.
//
// This helper uses `std::from_chars` (which never throws and never allocates)
// and returns `default_value` on empty input, non-numeric content, trailing
// garbage, or out-of-range values. A DEBUG log line records the bad value
// so operators can diagnose misbehaving clients via log-level toggle without
// dropping the surrounding packet.
//
// Field names are passed as string literals for the log message; callers
// should use the canonical AO field index (e.g. "MS[9]" for sfx_delay).
// ---------------------------------------------------------------------------
static int safe_stoi(std::string_view field, const char* field_name = "?", int default_value = 0) {
    if (field.empty())
        return default_value;
    int value = 0;
    const char* begin = field.data();
    const char* end = field.data() + field.size();
    auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc{} || ptr != end) {
        Log::log_print(DEBUG, "AO: non-numeric field %s=\"%.*s\" (using %d)", field_name,
                       static_cast<int>(field.size()), field.data(), default_value);
        return default_value;
    }
    return value;
}

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

static std::vector<std::string> ao_decode_list(const std::vector<std::string>& fields) {
    std::vector<std::string> out;
    out.reserve(fields.size());
    for (const auto& f : fields)
        out.push_back(ao_decode(f));
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
    // 3 fields = server→client: player_number, software, version
    // 2 fields = client→server: software, version
    if (fields.size() >= 3) {
        player_number = safe_stoi(fields[0], "ID[0]");
        software = fields[1];
        version = fields[2];
    }
    else if (fields.size() >= 2) {
        software = fields[0];
        version = fields[1];
    }
}

// ---------------------------------------------------------------------------
// AOPacketIDServer  (client → server, sending only)
// ---------------------------------------------------------------------------

AOPacketIDServer::AOPacketIDServer(const std::string& client_software, const std::string& client_version)
    : AOPacket("ID", {client_software, client_version}), client_software(client_software),
      client_version(client_version) {
}

// ---------------------------------------------------------------------------
// AOPacketPN
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketPN::registrar("PN", [](const auto& f) { return std::make_unique<AOPacketPN>(f); });

AOPacketPN::AOPacketPN(const std::vector<std::string>& fields) : AOPacket("PN", fields) {
    if (fields.size() >= MIN_FIELDS) {
        current_players = safe_stoi(fields[0], "PN[0]");
        max_players = safe_stoi(fields[1], "PN[1]");
        server_description = fields.size() > 2 ? ao_decode(fields[2]) : "";
    }
}

// ---------------------------------------------------------------------------
// AOPacketAskChaa  (client → server, no handle)
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketAskChaa::registrar("askchaa",
                                           [](const auto& f) { return std::make_unique<AOPacketAskChaa>(f); });

AOPacketAskChaa::AOPacketAskChaa() : AOPacket("askchaa", {}) {
}

AOPacketAskChaa::AOPacketAskChaa(const std::vector<std::string>& fields) : AOPacket("askchaa", fields) {
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
        character_count = safe_stoi(fields[0], "SI[0]");
        evidence_count = safe_stoi(fields[1], "SI[1]");
        music_count = safe_stoi(fields[2], "SI[2]");
    }
}

// ---------------------------------------------------------------------------
// AOPacketRC  (client → server, no handle)
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketRC::registrar("RC", [](const auto& f) { return std::make_unique<AOPacketRC>(f); });

AOPacketRC::AOPacketRC() : AOPacket("RC", {}) {
}

AOPacketRC::AOPacketRC(const std::vector<std::string>& fields) : AOPacket("RC", fields) {
}

// ---------------------------------------------------------------------------
// AOPacketSC
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketSC::registrar("SC", [](const auto& f) { return std::make_unique<AOPacketSC>(f); });

AOPacketSC::AOPacketSC(const std::vector<std::string>& fields)
    : AOPacket("SC", fields), character_list(ao_decode_list(fields)) {
}

// ---------------------------------------------------------------------------
// AOPacketRM  (client → server, no handle)
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketRM::registrar("RM", [](const auto& f) { return std::make_unique<AOPacketRM>(f); });

AOPacketRM::AOPacketRM() : AOPacket("RM", {}) {
}

AOPacketRM::AOPacketRM(const std::vector<std::string>& fields) : AOPacket("RM", fields) {
}

// ---------------------------------------------------------------------------
// AOPacketSM
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketSM::registrar("SM", [](const auto& f) { return std::make_unique<AOPacketSM>(f); });

AOPacketSM::AOPacketSM(const std::vector<std::string>& fields)
    : AOPacket("SM", fields), music_list(ao_decode_list(fields)) {
}

// ---------------------------------------------------------------------------
// AOPacketCH  (client → server keepalive, no handle)
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketCH::registrar("CH", [](const auto& f) { return std::make_unique<AOPacketCH>(f); });

AOPacketCH::AOPacketCH(int char_id) : AOPacket("CH", {std::to_string(char_id)}) {
}

AOPacketCH::AOPacketCH(const std::vector<std::string>& fields) : AOPacket("CH", fields) {
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

PacketRegistrar AOPacketRD::registrar("RD", [](const auto& f) { return std::make_unique<AOPacketRD>(f); });

AOPacketRD::AOPacketRD() : AOPacket("RD", {}) {
}

AOPacketRD::AOPacketRD(const std::vector<std::string>& fields) : AOPacket("RD", fields) {
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

PacketRegistrar AOPacketPW::registrar("PW", [](const auto& f) { return std::make_unique<AOPacketPW>(f); });

AOPacketPW::AOPacketPW(const std::string& password) : AOPacket("PW", {password}), password_(password) {
}

AOPacketPW::AOPacketPW(const std::vector<std::string>& fields) : AOPacket("PW", fields) {
    if (fields.size() >= MIN_FIELDS)
        password_ = fields[0];
}

// ---------------------------------------------------------------------------
// AOPacketCC
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketCC::registrar("CC", [](const auto& f) { return std::make_unique<AOPacketCC>(f); });

AOPacketCC::AOPacketCC(int player_num, int char_id, const std::string& hdid)
    : AOPacket("CC", {std::to_string(player_num), std::to_string(char_id), hdid}) {
}

AOPacketCC::AOPacketCC(const std::vector<std::string>& fields) : AOPacket("CC", fields) {
}

// ---------------------------------------------------------------------------
// AOPacketMS
// ---------------------------------------------------------------------------

#include "ao/event/OutgoingICMessageEvent.h"

PacketRegistrar AOPacketMS::registrar("MS", [](const auto& f) { return std::make_unique<AOPacketMS>(f); });

AOPacketMS::AOPacketMS(const ICMessageData& d)
    : AOPacket(
          "MS",
          {// 0-14: core fields
           std::to_string(d.desk_mod), ao_encode(d.pre_emote), ao_encode(d.character), ao_encode(d.emote),
           ao_encode(d.message), ao_encode(d.side), ao_encode(d.sfx_name), std::to_string(d.emote_mod),
           std::to_string(d.char_id), std::to_string(d.sfx_delay), std::to_string(d.objection_mod),
           std::to_string(d.evidence_id), std::to_string(d.flip), std::to_string(d.realization),
           std::to_string(d.text_color),
           // 15-18: 2.6 extensions (client→server format)
           ao_encode(d.showname), std::to_string(d.other_charid), ao_encode(d.self_offset), std::to_string(d.immediate),
           // 19-25: 2.8 extensions
           std::to_string(d.looping_sfx), std::to_string(d.screenshake), ao_encode(d.frame_screenshake),
           ao_encode(d.frame_realization), ao_encode(d.frame_sfx), std::to_string(d.additive), ao_encode(d.effects),
           // 26-27: blipname + slide (client→server only; pair fields are server-added)
           ao_encode(d.blipname), d.slide}) {
}

AOPacketMS::AOPacketMS(const std::vector<std::string>& fields) : AOPacket("MS", fields) {
    if (fields.size() >= MIN_FIELDS) {
        // "chat" means use default desk behavior based on position
        desk_mod = (fields[0] == "chat") ? -1 : safe_stoi(fields[0], "MS[0]");
        pre_emote = ao_decode(fields[1]);
        character = ao_decode(fields[2]);
        emote = ao_decode(fields[3]);
        message = ao_decode(fields[4]);
        side = ao_decode(fields[5]);
        sfx_name = ao_decode(fields[6]);
        emote_mod = safe_stoi(fields[7], "MS[7]");
        char_id = safe_stoi(fields[8], "MS[8]");
        // sfx_delay: webAO has been observed sending "NaN" here when the
        // char.ini's [SoundT] block is malformed. safe_stoi recovers with 0.
        sfx_delay = safe_stoi(fields[9], "MS[9]");
        objection_mod = safe_stoi(fields[10], "MS[10]");
        // fields[11] = evidence_id (skipped)
        flip = fields[12] == "1";
        realization = fields[13] == "1";
        text_color = safe_stoi(fields[14], "MS[14]");

        // Showname (optional field 15)
        showname = fields.size() > 15 ? ao_decode(fields[15]) : character;

        // 2.6 extensions (server→client echo format)
        // 22: immediate (no-int-pre)
        if (fields.size() > 22)
            immediate = fields[22] == "1";

        // 2.8 extensions (server→client echo format)
        // 23: looping_sfx, 24: screenshake, 25: frame_screenshake,
        // 26: frame_realization, 27: frame_sfx, 28: additive, 29: effects
        if (fields.size() > 23)
            sfx_looping = fields[23] == "1";
        if (fields.size() > 24)
            screenshake = fields[24] == "1";
        if (fields.size() > 25)
            frame_screenshake = ao_decode(fields[25]);
        if (fields.size() > 27)
            frame_sfx = ao_decode(fields[27]);
        if (fields.size() > 28)
            additive = fields[28] == "1";
        // fields[29] = effects, fields[30] = blipname (not yet used)
        if (fields.size() > 31)
            slide = fields[31] == "1";

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
    : AOPacket("MC", {ao_encode(name), std::to_string(char_id), ao_encode(showname), "1" /* looping */,
                      "0" /* channel */, "0" /* effects */}),
      name(name), char_id(char_id), showname(showname) {
}

AOPacketMC::AOPacketMC(const std::vector<std::string>& fields) : AOPacket("MC", fields) {
    if (fields.size() >= MIN_FIELDS) {
        name = ao_decode(fields[0]);
        char_id = safe_stoi(fields[1], "MC[1]");
        if (fields.size() > 2)
            showname = ao_decode(fields[2]);
        if (fields.size() > 3)
            looping = safe_stoi(fields[3], "MC[3]");
        if (fields.size() > 4)
            channel = safe_stoi(fields[4], "MC[4]");
        if (fields.size() > 5)
            effect_flags = safe_stoi(fields[5], "MC[5]");
    }
}

// ---------------------------------------------------------------------------
// AOPacketARUP
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketARUP::registrar("ARUP", [](const auto& f) { return std::make_unique<AOPacketARUP>(f); });

AOPacketARUP::AOPacketARUP(const std::vector<std::string>& fields) : AOPacket("ARUP", fields) {
    if (fields.size() >= MIN_FIELDS) {
        arup_type = safe_stoi(fields[0], "ARUP[0]");
        for (auto it = fields.begin() + 1; it != fields.end(); ++it)
            values.push_back(ao_decode(*it));
    }
}

// ---------------------------------------------------------------------------
// AOPacketBN
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketBN::registrar("BN", [](const auto& f) { return std::make_unique<AOPacketBN>(f); });

AOPacketBN::AOPacketBN(const std::vector<std::string>& fields) : AOPacket("BN", fields) {
    if (fields.size() >= MIN_FIELDS) {
        background = ao_decode(fields[0]);
        position = fields.size() >= 2 ? ao_decode(fields[1]) : "";
    }
}

// ---------------------------------------------------------------------------
// AOPacketPV
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketPV::registrar("PV", [](const auto& f) { return std::make_unique<AOPacketPV>(f); });

AOPacketPV::AOPacketPV(const std::vector<std::string>& fields) : AOPacket("PV", fields) {
    if (fields.size() >= MIN_FIELDS) {
        char_id = safe_stoi(fields[2], "PV[2]");
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

AOPacketFA::AOPacketFA(const std::vector<std::string>& fields) : AOPacket("FA", fields), areas(ao_decode_list(fields)) {
}

// ---------------------------------------------------------------------------
// AOPacketFM
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketFM::registrar("FM", [](const auto& f) { return std::make_unique<AOPacketFM>(f); });

AOPacketFM::AOPacketFM(const std::vector<std::string>& fields)
    : AOPacket("FM", fields), tracks(ao_decode_list(fields)) {
}

// ---------------------------------------------------------------------------
// AOPacketHP
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketHP::registrar("HP", [](const auto& f) { return std::make_unique<AOPacketHP>(f); });

AOPacketHP::AOPacketHP(const std::vector<std::string>& fields) : AOPacket("HP", fields) {
    if (fields.size() >= MIN_FIELDS) {
        side = safe_stoi(fields[0], "HP[0]");
        value = safe_stoi(fields[1], "HP[1]");
    }
}

// ---------------------------------------------------------------------------
// AOPacketTI
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketTI::registrar("TI", [](const auto& f) { return std::make_unique<AOPacketTI>(f); });

AOPacketTI::AOPacketTI(const std::vector<std::string>& fields) : AOPacket("TI", fields) {
    if (fields.size() >= MIN_FIELDS) {
        timer_id = safe_stoi(fields[0], "TI[0]");
        action = safe_stoi(fields[1], "TI[1]");
        if (fields.size() > 2) {
            // time_ms is int64 — use std::from_chars directly to avoid
            // truncating the value through safe_stoi's int path.
            long long parsed = 0;
            auto [p, ec] = std::from_chars(fields[2].data(), fields[2].data() + fields[2].size(), parsed);
            if (ec == std::errc{} && p == fields[2].data() + fields[2].size())
                time_ms = parsed;
            else
                Log::log_print(DEBUG, "AO: non-numeric field TI[2]=\"%s\" (using 0)", fields[2].c_str());
        }
    }
}

// ---------------------------------------------------------------------------
// AOPacketLE
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketLE::registrar("LE", [](const auto& f) { return std::make_unique<AOPacketLE>(f); });

AOPacketLE::AOPacketLE(const std::vector<std::string>& fields)
    : AOPacket("LE", fields), raw_items(ao_decode_list(fields)) {
}

// ---------------------------------------------------------------------------
// AOPacketPR
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketPR::registrar("PR", [](const auto& f) { return std::make_unique<AOPacketPR>(f); });

AOPacketPR::AOPacketPR(const std::vector<std::string>& fields) : AOPacket("PR", fields) {
    if (fields.size() >= MIN_FIELDS) {
        player_id = safe_stoi(fields[0], "PR[0]");
        update_type = safe_stoi(fields[1], "PR[1]");
    }
}

// ---------------------------------------------------------------------------
// AOPacketPU
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketPU::registrar("PU", [](const auto& f) { return std::make_unique<AOPacketPU>(f); });

AOPacketPU::AOPacketPU(const std::vector<std::string>& fields) : AOPacket("PU", fields) {
    if (fields.size() >= MIN_FIELDS) {
        player_id = safe_stoi(fields[0], "PU[0]");
        data_type = safe_stoi(fields[1], "PU[1]");
        data = ao_decode(fields[2]);
    }
}

// ---------------------------------------------------------------------------
// AOPacketZZ
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketZZ::registrar("ZZ", [](const auto& f) { return std::make_unique<AOPacketZZ>(f); });

AOPacketZZ::AOPacketZZ(const std::vector<std::string>& fields) : AOPacket("ZZ", fields) {
    if (!fields.empty())
        alert_reason = ao_decode(fields[0]);
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

// ---------------------------------------------------------------------------
// AOPacketMA
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketMA::registrar("MA", [](const auto& f) { return std::make_unique<AOPacketMA>(f); });

AOPacketMA::AOPacketMA(const std::vector<std::string>& fields) : AOPacket("MA", fields) {
    if (fields.size() >= MIN_FIELDS) {
        target_ipid = fields[0];
        duration = safe_stoi(fields[1], "MA[1]");
        reason = fields.size() > 2 ? ao_decode(fields[2]) : "No reason given";
    }
}

// ---------------------------------------------------------------------------
// AOPacketRT
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketRT::registrar("RT", [](const auto& f) { return std::make_unique<AOPacketRT>(f); });

AOPacketRT::AOPacketRT(const std::vector<std::string>& fields) : AOPacket("RT", fields) {
    if (!fields.empty())
        animation = fields[0];
}

// ---------------------------------------------------------------------------
// AOPacketPE
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketPE::registrar("PE", [](const auto& f) { return std::make_unique<AOPacketPE>(f); });

AOPacketPE::AOPacketPE(const std::vector<std::string>& fields) : AOPacket("PE", fields) {
    if (fields.size() >= MIN_FIELDS) {
        ev_name = ao_decode(fields[0]);
        ev_description = ao_decode(fields[1]);
        ev_image = ao_decode(fields[2]);
    }
}

// ---------------------------------------------------------------------------
// AOPacketEE
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketEE::registrar("EE", [](const auto& f) { return std::make_unique<AOPacketEE>(f); });

AOPacketEE::AOPacketEE(const std::vector<std::string>& fields) : AOPacket("EE", fields) {
    if (fields.size() >= MIN_FIELDS) {
        ev_id = safe_stoi(fields[0], "EE[0]", -1);
        ev_name = ao_decode(fields[1]);
        ev_description = ao_decode(fields[2]);
        ev_image = ao_decode(fields[3]);
    }
}

// ---------------------------------------------------------------------------
// AOPacketDE
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketDE::registrar("DE", [](const auto& f) { return std::make_unique<AOPacketDE>(f); });

AOPacketDE::AOPacketDE(const std::vector<std::string>& fields) : AOPacket("DE", fields) {
    if (fields.size() >= MIN_FIELDS) {
        ev_id = safe_stoi(fields[0], "DE[0]", -1);
    }
}

// ---------------------------------------------------------------------------
// AOPacketCASEA
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketCASEA::registrar("CASEA", [](const auto& f) { return std::make_unique<AOPacketCASEA>(f); });

AOPacketCASEA::AOPacketCASEA(const std::vector<std::string>& fields) : AOPacket("CASEA", fields) {
    if (fields.size() >= MIN_FIELDS) {
        case_title = ao_decode(fields[0]);
        need_def = fields[1] == "1";
        need_pro = fields[2] == "1";
        need_judge = fields[3] == "1";
        need_juror = fields[4] == "1";
        need_steno = fields[5] == "1";
    }
}

// ---------------------------------------------------------------------------
// AOPacketSETCASE
// ---------------------------------------------------------------------------

PacketRegistrar AOPacketSETCASE::registrar("SETCASE",
                                           [](const auto& f) { return std::make_unique<AOPacketSETCASE>(f); });

AOPacketSETCASE::AOPacketSETCASE(const std::vector<std::string>& fields) : AOPacket("SETCASE", fields) {
    if (fields.size() >= MIN_FIELDS) {
        showname = ao_decode(fields[0]);
        // fields[1] is ignored (duplicate casename in akashi)
        preferences.resize(5);
        preferences[0] = fields[2] == "1"; // def
        preferences[1] = fields[3] == "1"; // pro
        preferences[2] = fields[4] == "1"; // judge
        preferences[3] = fields[5] == "1"; // juror
        preferences[4] = fields[6] == "1"; // steno
    }
}

// Force this TU to be linked by providing a symbol that AOServer references.
void ao_register_packet_types() {
    // The static PacketRegistrar objects above run their constructors
    // at program startup, registering all packet types with the factory.
    // This function exists solely to create a linker dependency.
}
