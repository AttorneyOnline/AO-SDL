#pragma once

#include "PacketFactory.h"

class PacketRegistrar {
  public:
    PacketRegistrar(const std::string& header, PacketFactory::CreatorFunc creator) {
        PacketFactory::instance().register_packet(header, creator);
    }
};
