#include "game/BanManager.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

#include "utils/Log.h"

#include <chrono>


class BanCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "ban";
        return n;
    }

    bool requires_moderator() const override {
        return true;
    }

    int min_args() const override {
        return 2;
    }

    std::string usage() const override {
        return "/ban <ipid> <duration> [reason]";
    }

    void execute(CommandContext& ctx) override {
        auto* bm = ctx.room.ban_manager();
        if (!bm) {
            ctx.send_system_message("Ban system is not configured.");
            return;
        }

        auto& target_ipid = ctx.args[1];
        auto& duration_str = ctx.args[2];

        int64_t duration = parse_ban_duration(duration_str);
        if (duration == -1) {
            ctx.send_system_message(
                "Invalid duration: " + duration_str + ". Use 'perma', '30m', '1h', '2h30m', etc.");
            return;
        }

        // Build reason from remaining args
        std::string reason = "Banned";
        if (ctx.args.size() > 3) {
            reason.clear();
            for (size_t i = 3; i < ctx.args.size(); ++i) {
                if (!reason.empty())
                    reason += ' ';
                reason += ctx.args[i];
            }
        }

        // Find the HDID of the target (from any matching session)
        std::string target_hdid;
        ctx.room.for_each_session([&](ServerSession& session) {
            if (session.ipid == target_ipid && !session.hardware_id.empty())
                target_hdid = session.hardware_id;
        });

        // Create ban entry
        BanEntry entry;
        entry.ipid = target_ipid;
        entry.hdid = target_hdid;
        entry.reason = reason;
        entry.moderator = ctx.session.display_name;
        entry.timestamp =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
        entry.duration = duration;

        bm->add_ban(entry);

        // Kick all sessions matching this IPID
        int kicked = 0;
        std::string ban_msg = reason;
        if (!entry.is_permanent()) {
            ban_msg += " (duration: " + duration_str + ")";
        }
        else {
            ban_msg += " (permanent)";
        }

        ctx.room.for_each_session([&](ServerSession& session) {
            if (session.ipid == target_ipid) {
                ctx.send_ban_message(session.client_id, ban_msg);
                ++kicked;
            }
        });

        bm->save_async(DEFAULT_BAN_FILE);

        Log::log_print(INFO, "Ban: %s [%s] banned IPID %s (duration: %s, kicked %d): %s",
                       ctx.session.display_name.c_str(), ctx.session.ipid.c_str(), target_ipid.c_str(),
                       duration_str.c_str(), kicked, reason.c_str());
        ctx.send_system_message("Banned IPID " + target_ipid + " (" + duration_str + "), kicked " +
                                std::to_string(kicked) + " session(s).");
    }
};

static CommandRegistrar reg(std::make_unique<BanCommand>());
