/**
 * @file ServerList.h
 * @brief Parses a server list JSON document into ServerEntry structs.
 */
#pragma once

#include <json.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/**
 * @brief Describes a single game server entry from the master server list.
 */
struct ServerEntry {
    std::string hostname;                            /**< Server hostname or IP address. */
    std::optional<uint16_t> tcp_port = std::nullopt; /**< TCP port, if available. */
    std::optional<uint16_t> ws_port = std::nullopt;  /**< WebSocket port, if available. */
    std::optional<uint16_t> wss_port = std::nullopt; /**< Secure WebSocket port, if available. */

    std::string name;        /**< Human-readable server name. */
    std::string description; /**< Server description text. */
    int players;             /**< Current number of connected players. */

    /**
     * @brief Parse a single ServerEntry from a JSON object.
     * @param entry The JSON object representing one server.
     * @return A populated ServerEntry, or std::nullopt if the JSON is malformed.
     */
    static std::optional<ServerEntry> from_json(const nlohmann::json& entry);
};

/**
 * @brief Parses and holds the full list of servers from a JSON document.
 */
class ServerList {
  public:
    /**
     * @brief Construct a ServerList by parsing a JSON string.
     * @param serverlist_json Raw JSON string containing the server list array.
     */
    ServerList(const std::string& serverlist_json);

    /**
     * @brief Get the parsed list of servers.
     * @return A vector of all successfully parsed ServerEntry objects.
     */
    std::vector<ServerEntry> get_servers() const;

  private:
    std::vector<ServerEntry> servers; /**< Parsed server entries. */
};
