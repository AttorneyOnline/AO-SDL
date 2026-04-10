#include "game/ACLFlags.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "moderation/ContentModerator.h"
#include "utils/Log.h"

/// /modheat <ipid> [reset]
///
/// Without `reset`, prints the current heat + mute status for an IPID.
/// With `reset`, clears both the heat accumulator and any active mute
/// for that IPID (both in-memory and persistent via the mutes table).
///
/// Intended for the case where content moderation false-positives a
/// user and the human mod wants to clear their slate without waiting
/// for the decay half-life. Uses the MUTE ACL permission because
/// that's the closest existing permission bit — a future split could
/// add a dedicated MODERATION permission.
class ModHeatCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "modheat";
        return n;
    }

    ACLPermission required_permission() const override {
        return ACLPermission::MUTE;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/modheat <ipid> [reset]";
    }

    void execute(CommandContext& ctx) override {
        auto* cm = ctx.room.content_moderator();
        if (!cm) {
            ctx.send_system_message("Content moderation is not enabled on this server.");
            return;
        }

        const auto& target_ipid = ctx.args[1];
        const bool do_reset = ctx.args.size() >= 3 && ctx.args[2] == "reset";

        if (do_reset) {
            bool changed = cm->reset_state(target_ipid);
            Log::log_print(INFO, "ModHeat: %s [%s] cleared heat+mute for IPID %s (changed=%d)",
                           ctx.session.display_name.c_str(), ctx.session.ipid.c_str(),
                           target_ipid.c_str(), changed ? 1 : 0);
            ctx.send_system_message("Heat and mute cleared for IPID " + target_ipid +
                                    (changed ? "." : " (no prior state)."));
            return;
        }

        // Read-only status dump.
        const double heat = cm->current_heat(target_ipid);
        auto mute = cm->get_mute_info(target_ipid);

        std::string msg = "IPID " + target_ipid + ": heat=" + std::to_string(heat);
        if (mute) {
            msg += ", muted";
            if (!mute->reason.empty())
                msg += " (reason: " + mute->reason + ")";
            if (mute->seconds_remaining > 0)
                msg += ", " + std::to_string(mute->seconds_remaining) + "s remaining";
        }
        else {
            msg += ", not muted";
        }
        msg += ". Use `/modheat " + target_ipid + " reset` to clear.";
        ctx.send_system_message(msg);
    }
};

static CommandRegistrar reg(std::make_unique<ModHeatCommand>());

/// Anchor for RegisterCommands.cpp to drag this TU into the final link.
void ao_cmd_modheat() {
}
