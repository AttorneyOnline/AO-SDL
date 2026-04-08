#include "game/CommandRegistry.h"

#include "game/ACLFlags.h"
#include "game/ServerSession.h"
#include "utils/Log.h"

#include <sstream>

bool CommandRegistry::try_dispatch(CommandContext& ctx, const std::string& message) {
    if (message.empty() || message[0] != '/')
        return false;

    // Tokenize: "/cmd arg1 arg2 ..." → ["cmd", "arg1", "arg2", ...]
    std::istringstream stream(message.substr(1));
    ctx.args.clear();
    std::string token;
    while (stream >> token)
        ctx.args.push_back(std::move(token));

    if (ctx.args.empty())
        return false;

    // Lowercase the command name for case-insensitive lookup
    auto& cmd_name = ctx.args[0];
    for (auto& c : cmd_name)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    auto* handler = find(cmd_name);
    if (!handler) {
        ctx.send_system_message("Unknown command: /" + cmd_name + ". Type /help for a list.");
        return true; // consumed (don't broadcast the failed command)
    }

    // Permission check: ACL-based if the command declares a required_permission(),
    // otherwise falls back to the legacy requires_moderator() boolean.
    auto required = handler->required_permission();
    if (required != ACLPermission::NONE) {
        // Fine-grained ACL check
        if (!ctx.session.moderator) {
            ctx.send_system_message("Permission denied. You must be logged in.");
            return true;
        }
        auto session_perms = acl_permissions_for_role(ctx.session.acl_role);
        if (!has_permission(session_perms, required)) {
            ctx.send_system_message("Permission denied. You need the " + std::string(permission_name(required)) +
                                    " permission.");
            return true;
        }
    }
    else if (handler->requires_moderator() && !ctx.session.moderator) {
        ctx.send_system_message("Permission denied. You must be logged in as a moderator.");
        return true;
    }

    int arg_count = static_cast<int>(ctx.args.size()) - 1; // exclude command name
    if (arg_count < handler->min_args()) {
        ctx.send_system_message("Usage: " + handler->usage());
        return true;
    }

    handler->execute(ctx);

    Log::log_print(INFO, "Command: /%s from %s [%s]", cmd_name.c_str(), ctx.session.display_name.c_str(),
                   ctx.session.ipid.c_str());

    return true;
}
