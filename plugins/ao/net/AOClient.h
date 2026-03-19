#pragma once

#include "AOPacket.h"
#include "net/ProtocolHandler.h"

#include <string>
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

    // Used by packet handlers in PacketBehavior.cpp to queue outgoing packets.
    void add_message(const AOPacket& packet);

    AOConnectionState conn_state;

    std::string decryptor;
    int player_number;
    std::string server_software;
    std::string server_version;

    // todo: most of this state should move to events once the event system is more complete.
    int current_players;
    int max_players;
    std::string server_description;

    std::string asset_url;

    int character_count;
    int evidence_count;
    int music_count;

    std::vector<std::string> character_list;
    std::vector<std::string> music_list;

  private:
    std::string incomplete_buf;
    std::vector<std::string> buffered_messages;
};
