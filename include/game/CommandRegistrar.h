/**
 * @file CommandRegistrar.h
 * @brief Static initializer for command self-registration.
 *
 * Same pattern as PacketRegistrar: a static CommandRegistrar in each
 * command .cpp file registers the command at program startup.
 */
#pragma once

#include "game/CommandRegistry.h"

class CommandRegistrar {
  public:
    CommandRegistrar(std::unique_ptr<CommandHandler> cmd) {
        CommandRegistry::instance().register_command(std::move(cmd));
    }
};
