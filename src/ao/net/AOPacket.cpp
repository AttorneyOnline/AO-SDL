#include "AOPacket.h"

#include "AOClient.h"
#include "PacketFactory.h"
#include "PacketRegistrar.h"
#include "utils/Log.h"

#include <format>

AOPacket::AOPacket() : valid(false) {
}

AOPacket::AOPacket(std::string header, std::vector<std::string> fields) : valid(true), header(header), fields(fields) {
}

std::unique_ptr<AOPacket> AOPacket::deserialize(const std::string& serialized) {
    // Step 1: Validate the delimiter
    const std::string DELIMITER = "#%";
    if (serialized.size() < DELIMITER.size() || serialized.substr(serialized.size() - DELIMITER.size()) != DELIMITER) {
        throw std::invalid_argument("Invalid packet format: missing delimiter '#%'.");
    }

    // Step 2: Remove the delimiter
    std::string content = serialized.substr(0, serialized.size() - DELIMITER.size());

    // Step 3: Split the content by '#'
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t pos = content.find('#');

    while (pos != std::string::npos) {
        tokens.emplace_back(content.substr(start, pos - start));
        start = pos + 1;
        pos = content.find('#', start);
    }

    // Add the last token
    tokens.emplace_back(content.substr(start));

    if (tokens.empty()) {
        throw std::invalid_argument("Empty packet.");
    }

    // Step 4: Extract header and fields
    std::string header = tokens[0];
    std::vector<std::string> fields(tokens.begin() + 1, tokens.end());

    return PacketFactory::instance().create_packet(header, fields);
}

std::string AOPacket::serialize() const {
    std::string packet = header;

    for (auto field : fields) {
        packet = std::format("{}#{}", packet, field);
    }

    packet = std::format("{}#%", packet);
    return packet;
}

bool AOPacket::is_valid() {
    return valid;
}

void AOPacket::handle(AOClient& cli) {
    Log::log_print(WARNING, "Unhandled packet %s", serialize().c_str());
}