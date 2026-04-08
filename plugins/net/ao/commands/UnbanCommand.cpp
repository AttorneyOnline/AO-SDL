#include "game/BanManager.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"

#include "utils/Log.h"

class UnbanCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "unban";
        return n;
    }

    bool requires_moderator() const override {
        return true;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/unban <ipid>";
    }

    void execute(CommandContext& ctx) override {
        auto* bm = ctx.room.ban_manager();
        if (!bm) {
            ctx.send_system_message("Ban system is not configured.");
            return;
        }

        auto& target_ipid = ctx.args[1];

        if (bm->remove_ban(target_ipid)) {
            bm->save_async(DEFAULT_BAN_FILE);
            Log::log_print(INFO, "Unban: %s [%s] unbanned IPID %s", ctx.session.display_name.c_str(),
                           ctx.session.ipid.c_str(), target_ipid.c_str());
            ctx.send_system_message("Unbanned IPID " + target_ipid + ".");
        }
        else {
            ctx.send_system_message("No ban found for IPID " + target_ipid + ".");
        }
    }
};

static CommandRegistrar reg(std::make_unique<UnbanCommand>());
void ao_cmd_unban() {}
