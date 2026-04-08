/**
 * @file CommandHandler.h
 * @brief Abstract base for OOC slash commands.
 *
 * Each command is a subclass that registers itself via CommandRegistrar.
 * The server intercepts OOC messages starting with '/' and dispatches
 * them through CommandRegistry.
 */
#pragma once

#include "game/ACLFlags.h"

#include <string>

struct CommandContext;

class CommandHandler {
  public:
    virtual ~CommandHandler() = default;

    /// Command name (e.g. "kick"). Must be lowercase, no leading slash.
    virtual const std::string& name() const = 0;

    /// Whether this command requires moderator privileges.
    /// Default: derived from required_permission() — true if non-NONE.
    /// Existing commands may override this directly for backwards compat.
    virtual bool requires_moderator() const {
        return required_permission() != ACLPermission::NONE;
    }

    /// ACL permission required to execute this command.
    /// Default: NONE (anyone can run it). Override in auth-aware commands
    /// for fine-grained permission checks against the session's ACL role.
    virtual ACLPermission required_permission() const {
        return ACLPermission::NONE;
    }

    /// Minimum number of arguments (not counting the command name itself).
    virtual int min_args() const {
        return 0;
    }

    /// Short usage string shown in /help output (e.g. "/kick <ipid> [reason]").
    virtual std::string usage() const {
        return "/" + name();
    }

    /// Execute the command. args[0] is the command name.
    virtual void execute(CommandContext& ctx) = 0;
};
