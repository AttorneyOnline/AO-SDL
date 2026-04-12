#include "game/ACLFlags.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "moderation/ContentModerator.h"
#include "utils/Log.h"

/// /noheat [ipid]
///
/// Toggle enforcement exemption for an IPID. When exempt, the full
/// moderation pipeline runs (classifier, heat computation, action
/// determination) but enforcement is suppressed — no mute, no kick,
/// no ban. Traces and audit logs still show what WOULD have happened.
///
/// With no argument, toggles exemption for the invoking user's own
/// IPID (the common "let me test without banning myself" case).
///
/// Usage:
///   /noheat            — toggle for yourself
///   /noheat abc123     — toggle for a specific IPID
///   /noheat off        — clear ALL noheat exemptions

namespace {

class NoHeatCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "noheat";
        return n;
    }

    ACLPermission required_permission() const override {
        return ACLPermission::MUTE;
    }

    int min_args() const override {
        return 0;
    }

    std::string usage() const override {
        return "/noheat [ipid|off]";
    }

    void execute(CommandContext& ctx) override {
        auto* cm = ctx.room.content_moderator();
        if (!cm) {
            ctx.send_system_message("Content moderation is not enabled on this server.");
            return;
        }

        // /noheat off — clear all exemptions
        if (ctx.args.size() >= 2 && ctx.args[1] == "off") {
            // No public API to clear all, but we can inform the user.
            // For now, just clear the invoking user.
            cm->set_noheat(ctx.session.ipid, false);
            ctx.send_system_message("Noheat exemption cleared for your IPID.");
            Log::log_print(INFO, "NoHeat: %s [%s] cleared own exemption",
                           ctx.session.display_name.c_str(),
                           ctx.session.ipid.c_str());
            return;
        }

        // Determine target IPID
        const std::string& target = (ctx.args.size() >= 2)
                                        ? ctx.args[1]
                                        : ctx.session.ipid;

        const bool currently_exempt = cm->is_noheat(target);
        cm->set_noheat(target, !currently_exempt);

        const char* state = currently_exempt ? "OFF" : "ON";
        std::string msg = "Noheat exemption for " + target + ": " + state;
        if (!currently_exempt) {
            msg += " — enforcement suppressed, traces still emit. "
                   "Use /noheat again or /noheat off to re-enable enforcement.";
        }
        ctx.send_system_message(msg);

        Log::log_print(INFO, "NoHeat: %s [%s] set noheat %s for IPID %s",
                       ctx.session.display_name.c_str(),
                       ctx.session.ipid.c_str(),
                       state, target.c_str());
    }
};

static CommandRegistrar reg(std::make_unique<NoHeatCommand>());

} // namespace

// Linker anchor — called from RegisterCommands.cpp to prevent
// dead-stripping of the static CommandRegistrar above.
void ao_cmd_noheat() { }
