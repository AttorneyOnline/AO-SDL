#include "game/ACLFlags.h"
#include "game/BanManager.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"

#include <chrono>

class SearchBansCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "searchbans";
        return n;
    }

    ACLPermission required_permission() const override {
        return ACLPermission::BAN;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/searchbans <query> — search by IPID, HDID, reason, or moderator";
    }

    void execute(CommandContext& ctx) override {
        auto* bm = ctx.room.ban_manager();
        if (!bm) {
            ctx.send_system_message("Ban system is not configured.");
            return;
        }

        // Build query from all args after the command
        std::string query;
        for (size_t i = 1; i < ctx.args.size(); ++i) {
            if (!query.empty())
                query += ' ';
            query += ctx.args[i];
        }

        auto results = bm->search_bans(query);
        if (results.empty()) {
            ctx.send_system_message("No bans matching \"" + query + "\".");
            return;
        }

        ctx.send_system_message("Found " + std::to_string(results.size()) + " ban(s) matching \"" + query + "\":");

        auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                       .count();

        for (auto& ban : results) {
            std::string info = "  IPID: " + ban.ipid;
            if (!ban.hdid.empty())
                info += " | HDID: " + ban.hdid;
            info += " | " + format_ban_duration(ban.duration);
            if (!ban.is_permanent() && ban.duration > 0) {
                int64_t remaining = (ban.timestamp + ban.duration) - now;
                if (remaining > 0)
                    info += " (" + format_ban_duration(remaining) + " left)";
                else
                    info += " (expired)";
            }
            if (!ban.reason.empty())
                info += " | " + ban.reason;
            if (!ban.moderator.empty())
                info += " | by " + ban.moderator;
            ctx.send_system_message(info);
        }
    }
};

static CommandRegistrar reg(std::make_unique<SearchBansCommand>());
void ao_cmd_searchbans() {
}
