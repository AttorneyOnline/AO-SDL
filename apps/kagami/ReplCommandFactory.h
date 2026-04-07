#pragma once

#include "ReplCommand.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

/// Singleton registry of REPL command creators.
/// Mirrors EndpointFactory: plugins register creators at static-init time,
/// then populate() instantiates all commands and transfers them to the registry.
class ReplCommandFactory {
  public:
    using CreatorFunc = std::function<std::unique_ptr<ReplCommand>()>;

    static ReplCommandFactory& instance() {
        static ReplCommandFactory factory;
        return factory;
    }

    void register_command(const std::string& name, CreatorFunc creator) {
        creators_[name] = std::move(creator);
    }

    /// Instantiate all registered commands and add them to the registry.
    void populate(ReplCommandRegistry& registry);

  private:
    std::unordered_map<std::string, CreatorFunc> creators_;
};

/// Force-link all built-in command translation units.
void kagami_register_commands();
