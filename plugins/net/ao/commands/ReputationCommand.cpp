#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "game/IPReputationService.h"

class ReputationCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "reputation";
        return n;
    }

    bool requires_moderator() const override {
        return true;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/reputation <ip_or_ipid> | /reputation refresh <ip>";
    }

    void execute(CommandContext& ctx) override {
        auto* rep = ctx.room.reputation_service();
        if (!rep) {
            ctx.send_system_message("IP reputation service is not configured.");
            return;
        }

        auto& subcmd = ctx.args[1];

        if (subcmd == "refresh" && ctx.args.size() >= 3) {
            auto& ip = ctx.args[2];
            rep->lookup(ip, [](const IPReputationEntry&) {});
            ctx.send_system_message("Queued reputation refresh for " + ip);
            return;
        }

        auto& target = subcmd;
        auto cached = rep->find_cached(target);
        if (!cached) {
            ctx.send_system_message("No cached data for " + target + ". Use '/reputation refresh <ip>' to query.");
            return;
        }

        std::string msg = "Reputation for " + cached->ip + ":\n";
        msg += "  ASN: AS" + std::to_string(cached->asn) + " (" + cached->as_org + ")\n";
        msg += "  Country: " + cached->country_code + "\n";
        msg += "  ISP: " + cached->isp + "\n";
        msg += "  Proxy/VPN: " + std::string(cached->is_proxy ? "YES" : "no") + "\n";
        msg += "  Datacenter: " + std::string(cached->is_datacenter ? "YES" : "no") + "\n";
        if (cached->abuse_confidence > 0.0)
            msg += "  Abuse confidence: " + std::to_string(static_cast<int>(cached->abuse_confidence * 100)) + "%\n";
        msg += "  Cache size: " + std::to_string(rep->cache_size()) + " entries\n";
        msg += "  AbuseIPDB budget: " + std::to_string(rep->abuseipdb_budget_remaining()) + " remaining";

        ctx.send_system_message(msg);
    }
};

static CommandRegistrar reg(std::make_unique<ReputationCommand>());
