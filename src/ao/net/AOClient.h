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

  private:
    std::string incomplete_buf;
    std::vector<std::string> buffered_messages;
};

#endif