#include "ServerList.h"

#include <exception>
#include <optional>

#include "utils/JsonValidation.h"
#include "utils/Log.h"
#include "utils/json.hpp"

ServerList::ServerList(const std::string& serverlist_json) {
    nlohmann::basic_json respobj;
    try {
        respobj = nlohmann::json::parse(serverlist_json);
    }
    catch (nlohmann::json::exception exception) {
        throw exception;
    }

    for (const auto& serverobj : respobj) {
        std::optional<ServerEntry> entry;
        entry = ServerEntry::from_json(serverobj);

        if (entry) {
            servers.push_back(entry.value());
        }
    }
}

std::vector<ServerEntry> ServerList::get_servers() const {
    return servers;
}

std::optional<ServerEntry> ServerEntry::from_json(const nlohmann::json& server_data) {

    JsonValidation::ValidationSettings settings = {{"name", JsonValidation::ValueType::string},
                                                   {"description", JsonValidation::ValueType::string},
                                                   {"ip", JsonValidation::ValueType::string},
                                                   {"players", JsonValidation::ValueType::number_unsigned}};

    try {
        JsonValidation::containsKeys(server_data, settings);
    }
    catch (std::exception exception) {
        Log::log_print(DEBUG, exception.what());
        return std::nullopt;
    }

    ServerEntry server;
    server.name = server_data.at("name");
    server.description = server_data.at("description");
    server.hostname = server_data.at("ip");
    server.players = server_data.at("players");

    if (server_data.contains("port")) {
        server.tcp_port = std::make_optional<uint16_t>(server_data.at("port"));
    }

    if (server_data.contains("ws_port")) {
        server.ws_port = std::make_optional<uint16_t>(server_data.at("ws_port"));
    }

    if (server_data.contains("wss_port")) {
        server.wss_port = std::make_optional<uint16_t>(server_data.at("wss_port"));
    }

    return std::make_optional(server);
}
