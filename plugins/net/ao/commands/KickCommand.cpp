#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

#include "utils/Log.h"

class KickCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "kick";
        return n;
    }

    bool requires_moderator() const override {
        return true;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/kick <ipid> [reason]";
    }

    void execute(CommandContext& ctx) override {
        auto& target_ipid = ctx.args[1];

        // Build reason from remaining args
        std::string reason = "Kicked";
        if (ctx.args.size() > 2) {
            reason.clear();
            for (size_t i = 2; i < ctx.args.size(); ++i) {
                if (!reason.empty())
                    reason += ' ';
                reason += ctx.args[i];
            }
        }

        // Find and kick all sessions with this IPID
        int kicked = 0;
        ctx.room.for_each_session([&](ServerSession& session) {
            if (session.ipid == target_ipid) {
                ctx.send_kick_message(session.client_id, reason);
                ++kicked;
            }
        });

        if (kicked > 0) {
            Log::log_print(INFO, "Kick: %s [%s] kicked IPID %s (%d session(s)): %s", ctx.session.display_name.c_str(),
                           ctx.session.ipid.c_str(), target_ipid.c_str(), kicked, reason.c_str());
            ctx.send_system_message("Kicked " + std::to_string(kicked) + " session(s) for IPID " + target_ipid + ".");
        }
        else {
            ctx.send_system_message("No sessions found with IPID " + target_ipid + ".");
        }
    }
};

static CommandRegistrar reg(std::make_unique<KickCommand>());
void ao_cmd_kick() {
}
