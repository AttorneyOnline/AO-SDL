#include <gtest/gtest.h>

#include "net/ao/AOServer.h"
#include "net/ao/PacketTypes.h"

#include <string>
#include <vector>

class AOServerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        server_.set_send_func([this](uint64_t id, const std::string& data) { sent_packets_.push_back({id, data}); });

        auto& gs = server_.game_state();
        gs.characters = {"Phoenix", "Edgeworth", "Maya"};
        gs.music = {"Trial.opus", "Objection.opus"};
        gs.areas = {"Lobby", "Courtroom"};
        gs.server_name = "TestServer";
        gs.server_description = "A test server";
        gs.max_players = 10;
        gs.reset_taken();
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

    AOServer server_;
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
    auto* session = server_.get_session(id);
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

    auto* session = server_.get_session(id);
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

    EXPECT_TRUE(server_.game_state().char_taken[0]);
    server_.on_client_disconnected(id);
    EXPECT_FALSE(server_.game_state().char_taken[0]);
}

TEST_F(AOServerTest, SessionCount) {
    EXPECT_EQ(server_.session_count(), 0u);
    server_.on_client_connected(1);
    EXPECT_EQ(server_.session_count(), 1u);
    server_.on_client_connected(2);
    EXPECT_EQ(server_.session_count(), 2u);
    server_.on_client_disconnected(1);
    EXPECT_EQ(server_.session_count(), 1u);
}

TEST_F(AOServerTest, ICMessageBroadcastsToArea) {
    auto id1 = do_full_handshake();
    auto id2 = do_full_handshake();
    // Both in Lobby (default area)

    client_send(id1, "CC#0#0#hwid#%");
    clear_sent();

    // Client 1 sends IC message — should broadcast to both in Lobby
    std::string ms = "MS#0##Phoenix#normal#hello#def#Phoenix#5#0#0#0#0#0#0#0#%";
    client_send(id1, ms);

    auto pkts1 = packets_to(id1);
    auto pkts2 = packets_to(id2);
    // Both should receive the broadcast
    EXPECT_GE(pkts1.size(), 1u);
    EXPECT_GE(pkts2.size(), 1u);
}
