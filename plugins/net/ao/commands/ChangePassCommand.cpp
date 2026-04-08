#include "game/ACLFlags.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/DatabaseManager.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

#include "utils/Crypto.h"
#include "utils/Log.h"

/// /changepass <password> — Change your own password.
/// /changepass <username> <password> — Change another user's password (requires SUPER).
class ChangePassCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "changepass";
        return n;
    }

    bool requires_moderator() const override {
        return true;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/changepass <password>  OR  /changepass <username> <password>";
    }

    void execute(CommandContext& ctx) override {
        auto* db = ctx.room.db_manager();
        if (!db || !db->is_open()) {
            ctx.send_system_message("Database is not available.");
            return;
        }

        int arg_count = static_cast<int>(ctx.args.size()) - 1;
        std::string target_user;
        std::string new_password;

        if (arg_count == 1) {
            // Change own password
            if (ctx.session.moderator_name.empty()) {
                ctx.send_system_message("You are not logged in with a user account.");
                return;
            }
            target_user = ctx.session.moderator_name;
            new_password = ctx.args[1];
        }
        else {
            // Change another user's password — requires SUPER
            auto caller_perms = acl_permissions_for_role(ctx.session.acl_role);
            if (!has_permission(caller_perms, ACLPermission::SUPER)) {
                ctx.send_system_message("Permission denied. Changing another user's password requires SUPER.");
                return;
            }
            target_user = ctx.args[1];
            new_password = ctx.args[2];
        }

        if (new_password.size() < 8) {
            ctx.send_system_message("Password must be at least 8 characters.");
            return;
        }

        std::string salt = crypto::randbytes_hex(16);
        std::string hash = crypto::pbkdf2_sha256_hex(new_password, salt);

        bool updated = db->update_password(target_user, salt, hash).get();
        if (!updated) {
            ctx.send_system_message("User '" + target_user + "' not found.");
            return;
        }

        Log::log_print(INFO, "Auth: password changed for %s by %s", target_user.c_str(),
                       ctx.session.moderator_name.c_str());

        if (target_user == ctx.session.moderator_name)
            ctx.send_system_message("Your password has been changed.");
        else
            ctx.send_system_message("Password changed for " + target_user + ".");
    }
};

static CommandRegistrar reg(std::make_unique<ChangePassCommand>());
void ao_cmd_changepass() {
}
