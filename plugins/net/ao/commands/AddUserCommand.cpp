#include "game/ACLFlags.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/DatabaseManager.h"
#include "game/GameRoom.h"

#include "utils/Crypto.h"
#include "utils/Log.h"

/// /adduser <username> <password> — Create a new user with NONE permissions.
class AddUserCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "adduser";
        return n;
    }

    ACLPermission required_permission() const override {
        return ACLPermission::MODIFY_USERS;
    }

    int min_args() const override {
        return 2;
    }

    std::string usage() const override {
        return "/adduser <username> <password>";
    }

    void execute(CommandContext& ctx) override {
        auto* db = ctx.room.db_manager();
        if (!db || !db->is_open()) {
            ctx.send_system_message("Database is not available.");
            return;
        }

        auto& username = ctx.args[1];
        auto& password = ctx.args[2];

        if (password.size() < 8) {
            ctx.send_system_message("Password must be at least 8 characters.");
            return;
        }

        std::string salt = crypto::randbytes_hex(16);
        std::string hash = crypto::pbkdf2_sha256_hex(password, salt);

        bool created = db->create_user(username, salt, hash, "NONE").get();
        if (!created) {
            ctx.send_system_message("User '" + username + "' already exists.");
            return;
        }

        Log::log_print(INFO, "Auth: user '%s' created by %s", username.c_str(), ctx.session.moderator_name.c_str());
        ctx.send_system_message("User '" + username + "' created.");
    }
};

static CommandRegistrar reg(std::make_unique<AddUserCommand>());
void ao_cmd_adduser() {
}
