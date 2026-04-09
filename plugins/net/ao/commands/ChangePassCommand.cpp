#include "game/ACLFlags.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/DatabaseManager.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

#include "utils/Crypto.h"
#include "utils/Log.h"

/// /changepass <old_password> <new_password> — Change your own password.
/// /changepass <username> <new_password> — Change another user's password (requires SUPER).
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
        return 2;
    }

    std::string usage() const override {
        return "/changepass <old_password> <new_password>  OR  /changepass <username> <new_password>";
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

        if (arg_count == 2 && !ctx.session.moderator_name.empty()) {
            // Could be self-change or admin-change. Check if first arg
            // is a known username to disambiguate.
            auto caller_perms = acl_permissions_for_role(ctx.session.acl_role);
            auto target = db->get_user(ctx.args[1]).get();

            if (target && ctx.args[1] != ctx.session.moderator_name &&
                has_permission(caller_perms, ACLPermission::SUPER)) {
                // Admin changing another user's password
                target_user = ctx.args[1];
                new_password = ctx.args[2];
            }
            else {
                // Self-change: /changepass <old_password> <new_password>
                if (ctx.session.moderator_name.empty()) {
                    ctx.send_system_message("You are not logged in with a user account.");
                    return;
                }

                // Verify old password
                auto& old_password = ctx.args[1];
                auto user = db->get_user(ctx.session.moderator_name).get();
                if (!user) {
                    ctx.send_system_message("User account not found.");
                    return;
                }

                std::string computed;
                if (user->salt.size() >= 32)
                    computed = crypto::pbkdf2_sha256_hex(old_password, user->salt);
                else
                    computed = crypto::hmac_sha256_hex(user->salt, old_password);

                if (computed != user->password) {
                    ctx.send_system_message("Current password is incorrect.");
                    return;
                }

                target_user = ctx.session.moderator_name;
                new_password = ctx.args[2];
            }
        }
        else if (arg_count >= 2) {
            // Not logged in with a user account, or 3+ args — treat as admin change
            auto caller_perms = acl_permissions_for_role(ctx.session.acl_role);
            if (!has_permission(caller_perms, ACLPermission::SUPER)) {
                ctx.send_system_message("Permission denied. Changing another user's password requires SUPER.");
                return;
            }
            target_user = ctx.args[1];
            new_password = ctx.args[2];
        }
        else {
            ctx.send_system_message("Usage: " + usage());
            return;
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
