#pragma once

#include "net/EndpointFactory.h"

/// Static-init helper that registers a REST endpoint creator with
/// EndpointFactory. Parallels PacketRegistrar in the AO2 protocol layer.
///
/// Usage (in a .cpp file):
///   static EndpointRegistrar reg("GET /foo", [] {
///       return std::make_unique<FooEndpoint>();
///   });
class EndpointRegistrar {
  public:
    EndpointRegistrar(const std::string& key, EndpointFactory::CreatorFunc creator) {
        EndpointFactory::instance().register_endpoint(key, std::move(creator));
    }
};
