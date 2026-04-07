#pragma once

#include "ReplCommandFactory.h"

/// Static-init helper that registers a REPL command creator with
/// ReplCommandFactory. Mirrors EndpointRegistrar in the REST layer.
///
/// Usage (in a .cpp file):
///   static ReplCommandRegistrar reg("/foo", [] {
///       return std::make_unique<FooCommand>();
///   });
class ReplCommandRegistrar {
  public:
    ReplCommandRegistrar(const std::string& name, ReplCommandFactory::CreatorFunc creator) {
        ReplCommandFactory::instance().register_command(name, std::move(creator));
    }
};
