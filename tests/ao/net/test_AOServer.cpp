#include <gtest/gtest.h>

#include "game/GameRoom.h"
#include "net/ao/AOServer.h"
#include "net/ao/PacketTypes.h"
#include "utils/Log.h"

#include <string>
#include <vector>

class AOServerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Suppress verbose protocol logs during tests.
        Log::set_sink([](LogLevel, const std::string&, const std::string&) {});

        server_.set_send_func([this](uint64_t id, const std::string& data) { sent_packets_.push_back({id, data}); });

        room_.characters = {"Phoenix", "Edgeworth", "Maya"};
        room_.music = {"Trial.opus", "Objection.opus"};
        room_.areas = {"Lobby", "Courtroom"};
        room_.server_name = "TestServer";
        room_.server_description = "A test server";
        room_.max_players = 10;
        room_.reset_taken();
    }

    void TearDown() override {
        Log::set_sink(nullptr);
    }

    // Simulate a client connecting and sending a raw packet string.
    void client_send(uint64_t id, const std::string& packet) {
        server_.on_client_message(id, packet);
    }

    // Get all packets sent to a specific client, as raw strings.
    std::vector<std::string> packets_to(uint64_t id) {
        std::vector<std::string> result;
        for (auto& [cid, data] : sent_packets_) {
            if (cid == id)
                result.push_back(data);
        }
        return result;
    }

    // Clear sent packet log.
    void clear_sent() {
        sent_packets_.clear();
    }

    // Run full handshake for a client, return the client ID.
    uint64_t do_full_handshake() {
        uint64_t id = next_id_++;
        server_.on_client_connected(id);
        clear_sent(); // discard decryptor

        client_send(id, "HI#testhwid#%");
        clear_sent(); // discard ID response

        client_send(id, "ID#TestClient#2.10.0#%");
        clear_sent(); // discard PN + FL

        client_send(id, "askchaa#%");
        clear_sent();

        client_send(id, "RC#%");
        clear_sent();

        client_send(id, "RM#%");
        clear_sent();

        client_send(id, "RD#%");
        clear_sent();

        return id;
    }

    GameRoom room_;
    AOServer server_{room_};
    std::vector<std::pair<uint64_t, std::string>> sent_packets_;
    uint64_t next_id_ = 1;
};

TEST_F(AOServerTest, ConnectSendsDecryptor) {
    server_.on_client_connected(1);
    auto pkts = packets_to(1);
    ASSERT_GE(pkts.size(), 1u);
    EXPECT_EQ(pkts[0], "decryptor#NOENCRYPT#%");
}

TEST_F(AOServerTest, HIRespondsWithID) {
    server_.on_client_connected(1);
    clear_sent();

    client_send(1, "HI#myhardwareid#%");
    auto pkts = packets_to(1);
    ASSERT_GE(pkts.size(), 1u);
    EXPECT_NE(pkts[0].find("ID#"), std::string::npos);
    EXPECT_NE(pkts[0].find("kagami"), std::string::npos);
}

TEST_F(AOServerTest, IDRespondWithPNAndFL) {
    server_.on_client_connected(1);
    clear_sent();
    client_send(1, "HI#hwid#%");
    clear_sent();

    client_send(1, "ID#AO2#2.10.0#%");
    auto pkts = packets_to(1);
    ASSERT_GE(pkts.size(), 2u);
    EXPECT_NE(pkts[0].find("PN#"), std::string::npos);
    EXPECT_NE(pkts[0].find("10"), std::string::npos); // max_players
    EXPECT_NE(pkts[1].find("FL#"), std::string::npos);
    EXPECT_NE(pkts[1].find("noencryption"), std::string::npos);
}

TEST_F(AOServerTest, AskChaaRespondWithSI) {
    server_.on_client_connected(1);
    clear_sent();
    client_send(1, "HI#h#%");
    clear_sent();
    client_send(1, "ID#c#2.0.0#%");
    clear_sent();

    client_send(1, "askchaa#%");
    auto pkts = packets_to(1);
    ASSERT_GE(pkts.size(), 1u);
    // SI#char_count#evidence#music+areas
    // 3 chars, 0 evidence, 2 areas + 2 music = 4
    EXPECT_EQ(pkts[0], "SI#3#0#4#%");
}

TEST_F(AOServerTest, RCRespondWithSC) {
    server_.on_client_connected(1);
    clear_sent();
    client_send(1, "HI#h#%");
    clear_sent();
    client_send(1, "ID#c#2.0.0#%");
    clear_sent();
    client_send(1, "askchaa#%");
    clear_sent();

    client_send(1, "RC#%");
    auto pkts = packets_to(1);
    ASSERT_GE(pkts.size(), 1u);
    EXPECT_EQ(pkts[0], "SC#Phoenix#Edgeworth#Maya#%");
}

TEST_F(AOServerTest, RMRespondWithSM) {
    server_.on_client_connected(1);
    clear_sent();
    client_send(1, "HI#h#%");
    clear_sent();
    client_send(1, "ID#c#2.0.0#%");
    clear_sent();
    client_send(1, "askchaa#%");
    clear_sent();
    client_send(1, "RC#%");
    clear_sent();

    client_send(1, "RM#%");
    auto pkts = packets_to(1);
    ASSERT_GE(pkts.size(), 1u);
    EXPECT_EQ(pkts[0], "SM#Lobby#Courtroom#Trial.opus#Objection.opus#%");
}

TEST_F(AOServerTest, RDMarksJoinedAndSendsDone) {
    auto id = do_full_handshake();
    // After handshake, session should be joined
    auto* session = room_.get_session(id);
    ASSERT_NE(session, nullptr);
    EXPECT_TRUE(session->joined);
    EXPECT_EQ(session->area, "Lobby");
}

TEST_F(AOServerTest, FullHandshakePacketSequence) {
    server_.on_client_connected(1);

    // decryptor
    auto pkts = packets_to(1);
    ASSERT_EQ(pkts.size(), 1u);
    EXPECT_EQ(pkts[0], "decryptor#NOENCRYPT#%");
    clear_sent();

    // HI → ID response
    client_send(1, "HI#hwid#%");
    pkts = packets_to(1);
    ASSERT_EQ(pkts.size(), 1u);
    clear_sent();

    // ID → PN + FL
    client_send(1, "ID#Client#2.10.0#%");
    pkts = packets_to(1);
    ASSERT_EQ(pkts.size(), 2u);
    clear_sent();

    // askchaa → SI
    client_send(1, "askchaa#%");
    pkts = packets_to(1);
    ASSERT_EQ(pkts.size(), 1u);
    clear_sent();

    // RC → SC
    client_send(1, "RC#%");
    pkts = packets_to(1);
    ASSERT_EQ(pkts.size(), 1u);
    clear_sent();

    // RM → SM
    client_send(1, "RM#%");
    pkts = packets_to(1);
    ASSERT_EQ(pkts.size(), 1u);
    clear_sent();

    // RD → CharsCheck + DONE + BN + HP + HP + CT(motd)
    client_send(1, "RD#%");
    pkts = packets_to(1);
    ASSERT_GE(pkts.size(), 5u); // CharsCheck, DONE, BN, HP, HP, possibly CT
    EXPECT_NE(pkts[0].find("CharsCheck#"), std::string::npos);
    EXPECT_EQ(pkts[1], "DONE#%");
}

TEST_F(AOServerTest, CharacterSelection) {
    auto id = do_full_handshake();
    clear_sent();

    // Select character 1 (Edgeworth)
    client_send(id, "CC#0#1#hwid#%");
    auto pkts = packets_to(id);
    ASSERT_GE(pkts.size(), 1u);
    // PV response
    EXPECT_NE(pkts[0].find("PV#"), std::string::npos);
    EXPECT_NE(pkts[0].find("#CID#1#"), std::string::npos);

    auto* session = room_.get_session(id);
    EXPECT_EQ(session->character_id, 1);
    EXPECT_EQ(session->display_name, "Edgeworth");
}

TEST_F(AOServerTest, CharacterTakenRejected) {
    auto id1 = do_full_handshake();
    auto id2 = do_full_handshake();

    // Client 1 takes char 0
    client_send(id1, "CC#0#0#hwid#%");
    clear_sent();

    // Client 2 tries to take char 0
    client_send(id2, "CC#0#0#hwid#%");
    // Should NOT get a PV response (character taken)
    auto pkts = packets_to(id2);
    for (auto& p : pkts) {
        EXPECT_EQ(p.find("PV#"), std::string::npos) << "Should not get PV for taken character";
    }
}

TEST_F(AOServerTest, KeepaliveRespondsWithCheck) {
    auto id = do_full_handshake();
    clear_sent();

    client_send(id, "CH#0#%");
    auto pkts = packets_to(id);
    ASSERT_GE(pkts.size(), 1u);
    EXPECT_EQ(pkts[0], "CHECK#%");
}

TEST_F(AOServerTest, DisconnectFreesCharacter) {
    auto id = do_full_handshake();
    client_send(id, "CC#0#0#hwid#%");

    EXPECT_NE(room_.char_taken[0], 0);
    server_.on_client_disconnected(id);
    EXPECT_EQ(room_.char_taken[0], 0);
}

TEST_F(AOServerTest, SessionCount) {
    EXPECT_EQ(room_.session_count(), 0u);
    server_.on_client_connected(1);
    EXPECT_EQ(room_.session_count(), 1u);
    server_.on_client_connected(2);
    EXPECT_EQ(room_.session_count(), 2u);
    server_.on_client_disconnected(1);
    EXPECT_EQ(room_.session_count(), 1u);
}

TEST_F(AOServerTest, ICMessageBroadcastsToArea) {
    auto id1 = do_full_handshake();
    auto id2 = do_full_handshake();

    client_send(id1, "CC#0#0#hwid#%");
    clear_sent();

    std::string ms = "MS#0##Phoenix#normal#hello#def#Phoenix#5#0#0#0#0#0#0#0#%";
    client_send(id1, ms);

    auto pkts1 = packets_to(id1);
    auto pkts2 = packets_to(id2);
    EXPECT_GE(pkts1.size(), 1u);
    EXPECT_GE(pkts2.size(), 1u);
}

TEST_F(AOServerTest, ICMessagePreservesAllExtensionFields) {
    auto id1 = do_full_handshake();
    auto id2 = do_full_handshake();

    client_send(id1, "CC#0#0#hwid#%");
    clear_sent();

    // Full 28-field client→server MS with all extensions populated.
    // Layout: desk_mod, pre_emote, character, emote, message, side, sfx_name,
    //   emote_mod, char_id, sfx_delay, objection_mod, evidence_id, flip,
    //   realization, text_color, showname, other_charid, self_offset,
    //   immediate, looping_sfx, screenshake, frame_screenshake,
    //   frame_realization, frame_sfx, additive, effects, blipname, slide
    std::string ms = "MS#1#pre#Phoenix#talk#Hello world#def#slam.opus#5#0#200#2#3#1#1#4#"
                     "Nick#-1#40#1#1#1#FrameShake#FrameReal#FrameSfx#1#flash#male#1#%";
    client_send(id1, ms);

    auto pkts = packets_to(id2);
    ASSERT_GE(pkts.size(), 1u);

    // Find the MS echo (may be preceded by PU broadcasts).
    std::string echo;
    for (auto& p : pkts) {
        if (p.find("MS#") != std::string::npos) {
            echo = p;
            break;
        }
    }
    ASSERT_FALSE(echo.empty()) << "No MS packet found in broadcast";

    // Split the echo on '#' to check field positions
    std::vector<std::string> ef;
    size_t start = 0, pos;
    while ((pos = echo.find('#', start)) != std::string::npos) {
        ef.push_back(echo.substr(start, pos - start));
        start = pos + 1;
    }

    // Server→client echo layout (32 fields + header "MS"):
    // ef[0]=MS, ef[1..32]=fields, ef[33]=% (delimiter tail)
    ASSERT_GE(ef.size(), 32u);

    EXPECT_EQ(ef[1], "1");           // desk_mod
    EXPECT_EQ(ef[2], "pre");         // pre_emote
    EXPECT_EQ(ef[3], "Phoenix");     // character
    EXPECT_EQ(ef[4], "talk");        // emote
    EXPECT_EQ(ef[5], "Hello world"); // message
    EXPECT_EQ(ef[6], "def");         // side
    EXPECT_EQ(ef[7], "slam.opus");   // sfx_name
    EXPECT_EQ(ef[8], "5");           // emote_mod
    EXPECT_EQ(ef[9], "0");           // char_id
    EXPECT_EQ(ef[10], "200");        // sfx_delay
    EXPECT_EQ(ef[11], "2");          // objection_mod
    EXPECT_EQ(ef[12], "3");          // evidence_id
    EXPECT_EQ(ef[13], "1");          // flip
    EXPECT_EQ(ef[14], "1");          // realization
    EXPECT_EQ(ef[15], "4");          // text_color
    EXPECT_EQ(ef[16], "Nick");       // showname
    EXPECT_EQ(ef[17], "-1");         // other_charid (pair)
    // ef[18], ef[19], ef[20] = pair name, pair emote, self_offset
    EXPECT_EQ(ef[20], "40"); // self_offset
    // ef[21], ef[22] = pair offset, pair flip
    EXPECT_EQ(ef[23], "1");          // immediate
    EXPECT_EQ(ef[24], "1");          // sfx_looping
    EXPECT_EQ(ef[25], "1");          // screenshake
    EXPECT_EQ(ef[26], "FrameShake"); // frame_screenshake
    EXPECT_EQ(ef[27], "FrameReal");  // frame_realization
    EXPECT_EQ(ef[28], "FrameSfx");   // frame_sfx
    EXPECT_EQ(ef[29], "1");          // additive
    EXPECT_EQ(ef[30], "flash");      // effects
    EXPECT_EQ(ef[31], "male");       // blipname
    EXPECT_EQ(ef[32], "1");          // slide
}
