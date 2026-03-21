#pragma once

#include "AOPacket.h"
#include "net/ProtocolHandler.h"

#include <chrono>
#include <string>
#include <cstring>
#include <vector>

enum AOConnectionState { NOT_CONNECTED, CONNECTED, JOINED };

class AOClient : public ProtocolHandler {
  public:
    AOClient();

    // ProtocolHandler interface
    void on_connect() override;
    void on_message(const std::string& msg) override;
    void on_disconnect() override;
    std::vector<std::string> flush_outgoing() override;

    /// Queue an outgoing packet (used by packet handlers).
    void add_message(const AOPacket& packet);

    // --- Protocol state (written by packet handlers, internal to network thread) ---

    AOConnectionState conn_state = NOT_CONNECTED;

    std::string decryptor;
    int player_number = 0;

    std::string asset_url;

    int character_count = 0;
    int evidence_count = 0;
    int music_count = 0;

    std::vector<std::string> character_list;
    std::vector<std::string> music_list;

    int char_id = -1; // Set by PV packet when character is confirmed

    std::vector<std::string> features; // Server feature list (FL packet)

  private:
    std::string incomplete_buf;
    std::vector<std::string> buffered_messages;

    // Keepalive: send CH every 45 seconds to prevent proxy/server timeouts
    std::chrono::steady_clock::time_point last_keepalive_{};
    static constexpr int KEEPALIVE_INTERVAL_MS = 45000;
};
