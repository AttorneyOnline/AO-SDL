#include "game/ACLFlags.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/DatabaseManager.h"
#include "game/GameRoom.h"

#include "utils/Log.h"

/// /removeuser <username> — Delete a user account.
class RemoveUserCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "removeuser";
        return n;
    }

    ACLPermission required_permission() const override {
        return ACLPermission::MODIFY_USERS;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/removeuser <username>";
    }

    void execute(CommandContext& ctx) override {
        auto* db = ctx.room.db_manager();
        if (!db || !db->is_open()) {
            ctx.send_system_message("Database is not available.");
            return;
        }

        auto& username = ctx.args[1];

        // Protect the root user from deletion (matches akashi behavior)
        if (username == "root") {
            ctx.send_system_message("Cannot delete the root user.");
            return;
        }

        bool deleted = db->delete_user(username).get();
        if (!deleted) {
            ctx.send_system_message("User '" + username + "' not found.");
            return;
        }

        Log::log_print(INFO, "Auth: user '%s' deleted by %s", username.c_str(),
                       ctx.session.moderator_name.c_str());
        ctx.send_system_message("User '" + username + "' deleted.");
    }
};

static CommandRegistrar reg(std::make_unique<RemoveUserCommand>());
void ao_cmd_removeuser() {}
