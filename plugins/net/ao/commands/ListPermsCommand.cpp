#include "game/ACLFlags.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/DatabaseManager.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

/// /listperms [username] — Show permissions for yourself or another user.
class ListPermsCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "listperms";
        return n;
    }

    // Anyone can see their own permissions (NONE required).
    // Viewing others' requires MODIFY_USERS, checked in execute().

    std::string usage() const override {
        return "/listperms [username]";
    }

    void execute(CommandContext& ctx) override {
        int arg_count = static_cast<int>(ctx.args.size()) - 1;

        if (arg_count == 0) {
            // Show own permissions
            if (!ctx.session.moderator) {
                ctx.send_system_message("You are not logged in.");
                return;
            }

            auto perms = acl_permissions_for_role(ctx.session.acl_role);
            ctx.send_system_message("Your role: " + ctx.session.acl_role + "\n" + format_permissions(perms));
            return;
        }

        // Viewing another user's permissions requires MODIFY_USERS
        auto caller_perms = acl_permissions_for_role(ctx.session.acl_role);
        if (!ctx.session.moderator || !has_permission(caller_perms, ACLPermission::MODIFY_USERS)) {
            ctx.send_system_message("Permission denied. You need the MODIFY_USERS permission.");
            return;
        }

        auto* db = ctx.room.db_manager();
        if (!db || !db->is_open()) {
            ctx.send_system_message("Database is not available.");
            return;
        }

        auto& username = ctx.args[1];
        auto user_opt = db->get_user(username).get();
        if (!user_opt) {
            ctx.send_system_message("User '" + username + "' not found.");
            return;
        }

        auto perms = acl_permissions_for_role(user_opt->acl);
        ctx.send_system_message(username + "'s role: " + user_opt->acl + "\n" + format_permissions(perms));
    }

  private:
    static std::string format_permissions(ACLPermission perms) {
        if (perms == ACLPermission::SUPER)
            return "Permissions: ALL (SUPER)";
        if (perms == ACLPermission::NONE)
            return "Permissions: NONE";

        std::string result = "Permissions:";
        auto check = [&](ACLPermission p) {
            if (has_permission(perms, p))
                result += std::string("\n  ") + permission_name(p);
        };

        check(ACLPermission::KICK);
        check(ACLPermission::BAN);
        check(ACLPermission::BGLOCK);
        check(ACLPermission::MODIFY_USERS);
        check(ACLPermission::CM);
        check(ACLPermission::GLOBAL_TIMER);
        check(ACLPermission::EVI_MOD);
        check(ACLPermission::MOTD);
        check(ACLPermission::ANNOUNCE);
        check(ACLPermission::MODCHAT);
        check(ACLPermission::MUTE);
        check(ACLPermission::UNCM);
        check(ACLPermission::SAVETEST);
        check(ACLPermission::FORCE_CHARSELECT);
        check(ACLPermission::BYPASS_LOCKS);
        check(ACLPermission::IGNORE_BGLIST);
        check(ACLPermission::SEND_NOTICE);
        check(ACLPermission::JUKEBOX);

        return result;
    }
};

static CommandRegistrar reg(std::make_unique<ListPermsCommand>());
void ao_cmd_listperms() {
}
