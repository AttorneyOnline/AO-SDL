#include "game/BanManager.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/FirewallManager.h"
#include "game/GameRoom.h"

#include "utils/Log.h"

#include <string>

class FirewallCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "firewall";
        return n;
    }

    bool requires_moderator() const override {
        return true;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/firewall list | /firewall add <ip> <duration> | /firewall remove <ip> | /firewall flush";
    }

    void execute(CommandContext& ctx) override {
        auto* fw = ctx.room.firewall();
        if (!fw || !fw->is_enabled()) {
            ctx.send_system_message("Firewall manager is not configured or helper binary not found.");
            return;
        }

        auto& subcmd = ctx.args[1];

        if (subcmd == "list") {
            auto rules = fw->list_rules();
            if (rules.empty()) {
                ctx.send_system_message("No active firewall rules.");
                return;
            }

            std::string msg = std::to_string(rules.size()) + " active rule(s):\n";
            for (auto& rule : rules) {
                msg += "  " + rule.target + " — " + rule.reason;
                if (rule.expires_at > 0)
                    msg += " (expires)";
                else
                    msg += " (permanent)";
                msg += "\n";
            }
            ctx.send_system_message(msg);
        }
        else if (subcmd == "add" && ctx.args.size() >= 4) {
            auto& target = ctx.args[2];
            auto& duration_str = ctx.args[3];

            int64_t duration = parse_ban_duration(duration_str);
            if (duration == -1) {
                ctx.send_system_message("Invalid duration: " + duration_str);
                return;
            }

            std::string reason = "Manual firewall block by " + ctx.session.display_name;
            bool ok = fw->block_ip(target, reason, duration);
            if (ok) {
                fw->save_sync("firewall_rules.json");
                ctx.send_system_message("Firewall: blocked " + target + " (" + duration_str + ")");
                Log::log_print(INFO, "Firewall: %s blocked %s (%s)", ctx.session.display_name.c_str(), target.c_str(),
                               duration_str.c_str());
            }
            else {
                ctx.send_system_message("Failed to block " + target + " — invalid IP?");
            }
        }
        else if (subcmd == "remove" && ctx.args.size() >= 3) {
            auto& target = ctx.args[2];
            if (fw->unblock_ip(target)) {
                fw->save_sync("firewall_rules.json");
                ctx.send_system_message("Firewall: unblocked " + target);
            }
            else {
                ctx.send_system_message("No firewall rule for " + target);
            }
        }
        else if (subcmd == "flush") {
            fw->flush();
            fw->save_sync("firewall_rules.json");
            ctx.send_system_message("Firewall: all rules flushed.");
            Log::log_print(INFO, "Firewall: %s flushed all rules", ctx.session.display_name.c_str());
        }
        else {
            ctx.send_system_message("Usage: " + usage());
        }
    }
};

static CommandRegistrar reg(std::make_unique<FirewallCommand>());
void ao_cmd_firewall() {}
