#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

#include "utils/Log.h"

class LogoutCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "logout";
        return n;
    }

    bool requires_moderator() const override {
        return true;
    }

    std::string usage() const override {
        return "/logout";
    }

    void execute(CommandContext& ctx) override {
        ctx.session.moderator = false;
        ctx.session.acl_role.clear();
        ctx.session.moderator_name.clear();
        ctx.session.change_auth_started = false;
        ctx.room.stats.moderators.fetch_sub(1, std::memory_order_relaxed);

        Log::log_print(INFO, "Auth: %s [%s] logged out", ctx.session.display_name.c_str(), ctx.session.ipid.c_str());
        ctx.send_system_message("Logged out.");
    }
};

static CommandRegistrar reg(std::make_unique<LogoutCommand>());
