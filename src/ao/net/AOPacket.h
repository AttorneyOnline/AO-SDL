#ifndef AOPACKET_H
#define AOPACKET_H

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class AOClient;

class PacketFormatException : public std::invalid_argument {
  public:
    explicit PacketFormatException(const std::string& message) : std::invalid_argument(message){};
};

class ProtocolStateException : public std::runtime_error {
  public:
    explicit ProtocolStateException(const std::string& message) : std::runtime_error(message){};
};

class AOPacket {
  public:
    AOPacket();
    AOPacket(std::string header, std::vector<std::string> fields);

    std::string serialize() const;
    static std::unique_ptr<AOPacket> deserialize(const std::string& serialized);
    bool is_valid();

    virtual void handle(AOClient& cli);

    static constexpr const char* DELIMITER = "#%";

  protected:
    bool valid;

    std::string header;
    std::vector<std::string> fields;
};

#endif