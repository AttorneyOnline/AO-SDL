#include "game/ACLFlags.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/DatabaseManager.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

#include "utils/Crypto.h"
#include "utils/Log.h"

/// /rootpass <password> — Create the root account and switch to ADVANCED auth.
/// Only available after /changeauth has been initiated.
class RootPassCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "rootpass";
        return n;
    }

    bool requires_moderator() const override {
        return true;
    }

    ACLPermission required_permission() const override {
        return ACLPermission::SUPER;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/rootpass <password>";
    }

    void execute(CommandContext& ctx) override {
        if (!ctx.session.change_auth_started) {
            ctx.send_system_message("You must run /changeauth first.");
            return;
        }

        auto* db = ctx.room.db_manager();
        if (!db || !db->is_open()) {
            ctx.send_system_message("Database is not available.");
            return;
        }

        auto& password = ctx.args[1];
        if (password.size() < 8) {
            ctx.send_system_message("Password must be at least 8 characters.");
            return;
        }

        // Generate salt and hash the password (PBKDF2-SHA256, 100k iterations)
        std::string salt = crypto::randbytes_hex(16);
        std::string hash = crypto::pbkdf2_sha256_hex(password, salt);

        // Create root user with SUPER role
        bool created = db->create_user("root", salt, hash, "SUPER").get();
        if (!created) {
            ctx.send_system_message("Failed to create root user (may already exist).");
            return;
        }

        // Switch to ADVANCED auth
        ctx.room.auth_type = AuthType::ADVANCED;
        ctx.session.change_auth_started = false;

        // Force re-login
        ctx.session.moderator = false;
        ctx.session.acl_role.clear();
        ctx.session.moderator_name.clear();
        ctx.room.stats.moderators.fetch_sub(1, std::memory_order_relaxed);

        Log::log_print(INFO, "Auth: %s [%s] switched to ADVANCED auth, root account created",
                       ctx.session.display_name.c_str(), ctx.session.ipid.c_str());
        ctx.send_system_message("Root account created. Authentication switched to ADVANCED mode.\n"
                                "You have been logged out. Use /login root <password> to log back in.");
    }
};

static CommandRegistrar reg(std::make_unique<RootPassCommand>());
void ao_cmd_rootpass() {
}
