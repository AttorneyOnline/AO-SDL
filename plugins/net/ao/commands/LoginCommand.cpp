#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

#include "utils/Log.h"

class LoginCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "login";
        return n;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/login <password>";
    }

    void execute(CommandContext& ctx) override {
        if (ctx.session.moderator) {
            ctx.send_system_message("You are already logged in.");
            return;
        }

        auto& configured = ctx.room.mod_password;
        if (configured.empty()) {
            ctx.send_system_message("Authentication is not configured on this server.");
            return;
        }

        if (ctx.args[1] != configured) {
            Log::log_print(WARNING, "Auth: failed login attempt from %s [%s]", ctx.session.display_name.c_str(),
                           ctx.session.ipid.c_str());
            ctx.send_system_message("Invalid password.");
            return;
        }

        ctx.session.moderator = true;
        ctx.room.stats.moderators.fetch_add(1, std::memory_order_relaxed);

        Log::log_print(INFO, "Auth: %s [%s] logged in as moderator", ctx.session.display_name.c_str(),
                       ctx.session.ipid.c_str());
        ctx.send_system_message("Logged in as moderator.");
    }
};

static CommandRegistrar reg(std::make_unique<LoginCommand>());
