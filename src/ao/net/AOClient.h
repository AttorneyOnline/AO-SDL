#ifndef AOCLIENT_H
#define AOCLIENT_H

#include <string>
#include <vector>

#include "AOPacket.h"

enum AOConnectionState { NOT_CONNECTED, CONNECTED, JOINED };

class AOClient {
  public:
    AOClient();

    std::vector<std::string> get_messages();
    void handle_message(const std::string& message);
    void add_message(const AOPacket& packet);

    AOConnectionState conn_state;

    std::string decryptor;
    int player_number;
    std::string server_software;
    std::string server_version;

    // todo
    // a lot of this should probably not live here
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

#endif
