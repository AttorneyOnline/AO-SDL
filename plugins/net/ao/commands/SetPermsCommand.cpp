#include "game/ACLFlags.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/DatabaseManager.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

#include "utils/Log.h"

/// /setperms <username> <role> — Set ACL role for a user.
class SetPermsCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "setperms";
        return n;
    }

    ACLPermission required_permission() const override {
        return ACLPermission::MODIFY_USERS;
    }

    int min_args() const override {
        return 2;
    }

    std::string usage() const override {
        return "/setperms <username> <role>";
    }

    void execute(CommandContext& ctx) override {
        auto* db = ctx.room.db_manager();
        if (!db || !db->is_open()) {
            ctx.send_system_message("Database is not available.");
            return;
        }

        auto& username = ctx.args[1];
        auto& role = ctx.args[2];

        // Cannot set SUPER unless caller has SUPER
        auto caller_perms = acl_permissions_for_role(ctx.session.acl_role);
        if (acl_permissions_for_role(role) == ACLPermission::SUPER &&
            !has_permission(caller_perms, ACLPermission::SUPER)) {
            ctx.send_system_message("Only SUPER users can assign the SUPER role.");
            return;
        }

        // Protect root's role from being changed (matches akashi)
        if (username == "root") {
            ctx.send_system_message("Cannot change the root user's role.");
            return;
        }

        bool updated = db->update_acl(username, role).get();
        if (!updated) {
            ctx.send_system_message("User '" + username + "' not found.");
            return;
        }

        Log::log_print(INFO, "Auth: %s set %s's role to %s", ctx.session.moderator_name.c_str(), username.c_str(),
                       role.c_str());
        ctx.send_system_message("Set " + username + "'s role to " + role + ".");
    }
};

static CommandRegistrar reg(std::make_unique<SetPermsCommand>());
void ao_cmd_setperms() {}
