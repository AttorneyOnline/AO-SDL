#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class AOClient;
class AOServer;
struct ServerSession;

class PacketFormatException : public std::invalid_argument {
  public:
    explicit PacketFormatException(const std::string& message) : std::invalid_argument(message) {
    }
};

class ProtocolStateException : public std::runtime_error {
  public:
    explicit ProtocolStateException(const std::string& message) : std::runtime_error(message) {
    }
};

class AOPacket {
  public:
    AOPacket();
    AOPacket(std::string header, std::vector<std::string> fields);

    std::string serialize() const;
    static std::unique_ptr<AOPacket> deserialize(const std::string& serialized);
    bool is_valid();

    /// Client-side handler: process a packet received from the server.
    virtual void handle(AOClient& cli);

    /// Server-side handler: process a packet received from a client.
    virtual void handle_server(AOServer& server, ServerSession& session);

    static constexpr const char* DELIMITER = "#%";

    /// Public read-only access to the packet type (e.g. "MS", "CT", "CC").
    const std::string& get_header() const {
        return header;
    }

  protected:
    bool valid;

    std::string header;
    std::vector<std::string> fields;
};
