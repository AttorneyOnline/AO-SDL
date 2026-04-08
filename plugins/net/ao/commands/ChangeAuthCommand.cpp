#include "game/ACLFlags.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

/// /changeauth — Initiate migration from SIMPLE to ADVANCED authentication.
/// Requires SUPER permission. Sets a flag so /rootpass can proceed.
class ChangeAuthCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "changeauth";
        return n;
    }

    bool requires_moderator() const override {
        return true;
    }

    ACLPermission required_permission() const override {
        return ACLPermission::SUPER;
    }

    std::string usage() const override {
        return "/changeauth";
    }

    void execute(CommandContext& ctx) override {
        if (ctx.room.auth_type == AuthType::ADVANCED) {
            ctx.send_system_message("Already using advanced authentication.");
            return;
        }

        ctx.session.change_auth_started = true;
        ctx.send_system_message("WARNING: You are about to change the authentication system.\n"
                                "This will switch from a single shared password to per-user database auth.\n"
                                "Use /rootpass <password> to set the root account password and complete the switch.");
    }
};

static CommandRegistrar reg(std::make_unique<ChangeAuthCommand>());
void ao_cmd_changeauth() {
}
