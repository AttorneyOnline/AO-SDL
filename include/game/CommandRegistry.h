/**
 * @file CommandRegistry.h
 * @brief Singleton registry for OOC slash commands.
 *
 * Commands self-register at static init time via CommandRegistrar.
 * Protocol backends call try_dispatch() from their OOC handler;
 * it returns true if the message was a command (consumed), false
 * if it should be broadcast as normal OOC chat.
 */
#pragma once

#include "game/CommandContext.h"
#include "game/CommandHandler.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class CommandRegistry {
  public:
    static CommandRegistry& instance() {
        static CommandRegistry registry;
        return registry;
    }

    void register_command(std::unique_ptr<CommandHandler> cmd) {
        auto name = cmd->name();
        commands_[name] = std::move(cmd);
    }

    CommandHandler* find(const std::string& name) const {
        auto it = commands_.find(name);
        return it != commands_.end() ? it->second.get() : nullptr;
    }

    /// Try to dispatch an OOC message as a command.
    /// Returns true if the message was a command (starts with '/') and was
    /// handled (or an error was sent). Returns false if the message is
    /// normal chat and should be broadcast.
    bool try_dispatch(CommandContext& ctx, const std::string& message);

    /// Get all registered commands (for /help listing).
    std::vector<CommandHandler*> all_commands() const {
        std::vector<CommandHandler*> result;
        result.reserve(commands_.size());
        for (auto& [name, cmd] : commands_)
            result.push_back(cmd.get());
        return result;
    }

  private:
    CommandRegistry() = default;
    std::unordered_map<std::string, std::unique_ptr<CommandHandler>> commands_;
};
