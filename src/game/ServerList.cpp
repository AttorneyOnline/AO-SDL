#include "ServerList.h"

#include "utils/json.hpp"

#include <format>

ServerList::ServerList(const std::string& serverlist_json) {
    auto respobj = nlohmann::json::parse(serverlist_json);

    // todo: better validation!!! important!!
    for (auto serverobj : respobj) {
        ServerEntry entry;

        entry.name = serverobj["name"];
        entry.description = serverobj["description"];
        entry.hostname = serverobj["ip"];

        if (serverobj["ws_port"].is_number()) {
            entry.ws_port = serverobj["ws_port"];
        }
        else {
            entry.ws_port = 0;
            continue;
        }

        if (serverobj["players"].is_number()) {
            entry.players = serverobj["players"];
        }
        else {
            entry.players = -1;
            continue;
        }

        servers.push_back(entry);
    }
}

std::vector<ServerEntry> ServerList::get_servers() const {
    return servers;
}