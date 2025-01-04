#include "ServerList.h"

#include <optional>

#include "utils/JsonValidation.h"
#include "utils/Log.h"
#include "utils/json.hpp"

ServerList::ServerList(const std::string& serverlist_json) {
    nlohmann::basic_json respobj;
    try {
        respobj = nlohmann::json::parse(serverlist_json);
    }
    catch (nlohmann::json::exception e) {
        Log::log_print(FATAL, e.what());
        return;
    }

    int server_id = 0;
    for (const auto& serverobj : respobj) {
        std::optional<ServerEntry> entry = ServerEntry::from_json(serverobj, server_id);

        if (entry) {
            servers.push_back(entry.value());
        }
        server_id++;
    }
}

std::vector<ServerEntry> ServerList::get_servers() const {
    return servers;
}

std::optional<ServerEntry> ServerEntry::from_json(nlohmann::json server_data, int server_id) {

    JsonValidation::ValidationSettings settings = {{"name", JsonValidation::ValueType::string},
                                                   {"description", JsonValidation::ValueType::string},
                                                   {"ip", JsonValidation::ValueType::string},
                                                   {"players", JsonValidation::ValueType::number_unsigned}};

    if (!JsonValidation::containsKeys(server_data, settings)) {
        return std::nullopt;
    }

    ServerEntry server;
    server.name = server_data["name"];
    server.description = server_data["description"];
    server.hostname = server_data["ip"];
    server.players = server_data["players"];

    if (server_data.contains("port")) {
        server.tcp_port = std::make_optional<uint16_t>(server_data["port"]);
    }

    if (server_data.contains("ws_port")) {
        server.ws_port = std::make_optional<uint16_t>(server_data["ws_port"]);
    }

    if (server_data.contains("wss_port")) {
        server.wss_port = std::make_optional<uint16_t>(server_data["wss_port"]);
    }

    return std::make_optional(server);
}
