#ifndef SERVERLIST_H
#define SERVERLIST_H

#include <cstdint>
#include <string>
#include <vector>

struct ServerEntry {
    std::string hostname;
    uint16_t ws_port;

    std::string name;
    std::string description;
    int players;
};

class ServerList {
  public:
    ServerList(const std::string& serverlist_json);

    std::vector<ServerEntry> get_servers() const;

  private:
    std::vector<ServerEntry> servers;
};

#endif