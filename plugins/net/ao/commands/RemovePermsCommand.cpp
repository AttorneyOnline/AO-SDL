#include "game/ACLFlags.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/DatabaseManager.h"
#include "game/GameRoom.h"

#include "utils/Log.h"

/// /removeperms <username> — Reset a user's role to NONE.
class RemovePermsCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "removeperms";
        return n;
    }

    ACLPermission required_permission() const override {
        return ACLPermission::MODIFY_USERS;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/removeperms <username>";
    }

    void execute(CommandContext& ctx) override {
        auto* db = ctx.room.db_manager();
        if (!db || !db->is_open()) {
            ctx.send_system_message("Database is not available.");
            return;
        }

        auto& username = ctx.args[1];

        if (username == "root") {
            ctx.send_system_message("Cannot remove the root user's permissions.");
            return;
        }

        bool updated = db->update_acl(username, "NONE").get();
        if (!updated) {
            ctx.send_system_message("User '" + username + "' not found.");
            return;
        }

        Log::log_print(INFO, "Auth: %s removed permissions from %s", ctx.session.moderator_name.c_str(),
                       username.c_str());
        ctx.send_system_message("Removed permissions from " + username + ".");
    }
};

static CommandRegistrar reg(std::make_unique<RemovePermsCommand>());
void ao_cmd_removeperms() {}
