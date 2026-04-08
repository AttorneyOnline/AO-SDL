#include "game/ACLFlags.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/DatabaseManager.h"
#include "game/GameRoom.h"

/// /listusers — List all registered users.
class ListUsersCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "listusers";
        return n;
    }

    ACLPermission required_permission() const override {
        return ACLPermission::MODIFY_USERS;
    }

    std::string usage() const override {
        return "/listusers";
    }

    void execute(CommandContext& ctx) override {
        auto* db = ctx.room.db_manager();
        if (!db || !db->is_open()) {
            ctx.send_system_message("Database is not available.");
            return;
        }

        auto users = db->list_users().get();
        if (users.empty()) {
            ctx.send_system_message("No registered users.");
            return;
        }

        std::string msg = "Registered users (" + std::to_string(users.size()) + "):";
        for (auto& u : users)
            msg += "\n  " + u;
        ctx.send_system_message(msg);
    }
};

static CommandRegistrar reg(std::make_unique<ListUsersCommand>());
void ao_cmd_listusers() {
}
