#ifndef SERVERLIST_H
#define SERVERLIST_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "utils/json.hpp"

struct ServerEntry {
    std::string hostname;
    std::optional<uint16_t> tcp_port = std::nullopt;
    std::optional<uint16_t> ws_port = std::nullopt;
    std::optional<uint16_t> wss_port = std::nullopt;

    std::string name;
    std::string description;
    int players;

    static std::optional<ServerEntry> from_json(nlohmann::json entry, int server_id);
};

class ServerList {
  public:
    ServerList(const std::string& serverlist_json);

    std::vector<ServerEntry> get_servers() const;

  private:
    std::vector<ServerEntry> servers;
};

#endif
