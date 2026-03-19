#pragma once

#include "AOPacket.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class PacketFactory {
  public:
    using CreatorFunc = std::function<std::unique_ptr<AOPacket>(const std::vector<std::string>&)>;

    static PacketFactory& instance() {
        static PacketFactory factory;
        return factory;
    }

    void register_packet(const std::string& header, CreatorFunc creator) {
        creators[header] = creator;
    }

    std::unique_ptr<AOPacket> create_packet(const std::string& header, const std::vector<std::string>& fields) const {
        auto it = creators.find(header);
        if (it != creators.end()) {
            return it->second(fields);
        }
        // Handle unknown packet types as needed
        return std::make_unique<AOPacket>(header, fields);
    }

  private:
    std::unordered_map<std::string, CreatorFunc> creators;
};
