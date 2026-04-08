#include "game/ASNReputationManager.h"
#include "game/BanManager.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "game/IPReputationService.h"

#include "utils/Log.h"

#include <string>

class AsnCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "asn";
        return n;
    }

    bool requires_moderator() const override {
        return true;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/asn <ip_or_ipid> | /asn status <asn> | /asn block <asn> <duration> [reason] | /asn unblock <asn> | "
               "/asn list";
    }

    void execute(CommandContext& ctx) override {
        auto& subcmd = ctx.args[1];

        if (subcmd == "list") {
            do_list(ctx);
        }
        else if (subcmd == "status" && ctx.args.size() >= 3) {
            do_status(ctx);
        }
        else if (subcmd == "block" && ctx.args.size() >= 4) {
            do_block(ctx);
        }
        else if (subcmd == "unblock" && ctx.args.size() >= 3) {
            do_unblock(ctx);
        }
        else {
            // Default: lookup IP/IPID
            do_lookup(ctx);
        }
    }

  private:
    void do_lookup(CommandContext& ctx) {
        auto* rep = ctx.room.reputation_service();
        if (!rep) {
            ctx.send_system_message("IP reputation service is not configured.");
            return;
        }

        auto& target = ctx.args[1];
        // Try to find IP from IPID by checking connected sessions
        std::string ip = target;
        if (target.size() == 8) {
            // Might be an IPID — look up the IP
            ctx.room.for_each_session([&](ServerSession& session) {
                if (session.ipid == target)
                    ip = target; // We don't have raw IP from session, show IPID
            });
        }

        auto cached = rep->find_cached(ip);
        if (!cached) {
            ctx.send_system_message("No cached reputation data for " + target + ". Triggering lookup...");
            rep->lookup(ip);
            return;
        }

        std::string msg = "IP: " + cached->ip + "\n";
        msg += "ASN: AS" + std::to_string(cached->asn) + " (" + cached->as_org + ")\n";
        msg += "Country: " + cached->country_code + "\n";
        msg += "ISP: " + cached->isp + "\n";
        msg += "Proxy: " + std::string(cached->is_proxy ? "YES" : "no") + "\n";
        msg += "Datacenter: " + std::string(cached->is_datacenter ? "YES" : "no") + "\n";
        if (cached->abuse_confidence > 0.0)
            msg += "Abuse confidence: " + std::to_string(static_cast<int>(cached->abuse_confidence * 100)) + "%\n";

        // Also check ASN reputation
        if (auto* asn_mgr = ctx.room.asn_reputation()) {
            if (auto status = asn_mgr->get_status(cached->asn)) {
                msg += "ASN status: " + std::string(asn_status_to_string(status->status));
                if (!status->block_reason.empty())
                    msg += " (" + status->block_reason + ")";
                msg += "\n";
            }
        }

        ctx.send_system_message(msg);
    }

    void do_status(CommandContext& ctx) {
        auto* asn_mgr = ctx.room.asn_reputation();
        if (!asn_mgr) {
            ctx.send_system_message("ASN reputation manager is not configured.");
            return;
        }

        uint32_t asn = 0;
        try {
            asn = static_cast<uint32_t>(std::stoul(ctx.args[2]));
        }
        catch (...) {
            ctx.send_system_message("Invalid ASN number: " + ctx.args[2]);
            return;
        }

        auto status = asn_mgr->get_status(asn);
        if (!status) {
            ctx.send_system_message("AS" + std::to_string(asn) + ": no reputation data recorded.");
            return;
        }

        std::string msg = "AS" + std::to_string(asn) + " (" + status->as_org + ")\n";
        msg += "Status: " + std::string(asn_status_to_string(status->status)) + "\n";
        msg += "Total abuse events: " + std::to_string(status->total_abuse_events) + "\n";
        msg += "Unique abusive IPs (window): " + std::to_string(status->abusive_ips_in_window.size()) + "\n";
        if (!status->block_reason.empty())
            msg += "Block reason: " + status->block_reason + "\n";

        ctx.send_system_message(msg);
    }

    void do_block(CommandContext& ctx) {
        auto* asn_mgr = ctx.room.asn_reputation();
        if (!asn_mgr) {
            ctx.send_system_message("ASN reputation manager is not configured.");
            return;
        }

        uint32_t asn = 0;
        try {
            asn = static_cast<uint32_t>(std::stoul(ctx.args[2]));
        }
        catch (...) {
            ctx.send_system_message("Invalid ASN number: " + ctx.args[2]);
            return;
        }

        int64_t duration = parse_ban_duration(ctx.args[3]);
        if (duration == -1) {
            ctx.send_system_message("Invalid duration: " + ctx.args[3]);
            return;
        }

        std::string reason = "Manually blocked by " + ctx.session.display_name;
        if (ctx.args.size() > 4) {
            reason.clear();
            for (size_t i = 4; i < ctx.args.size(); ++i) {
                if (!reason.empty())
                    reason += ' ';
                reason += ctx.args[i];
            }
        }

        asn_mgr->block_asn(asn, "", reason, duration);
        asn_mgr->save_async("asn_reputation.json");

        ctx.send_system_message("Blocked AS" + std::to_string(asn) + " (" + ctx.args[3] + "): " + reason);

        Log::log_print(INFO, "ASN block: %s blocked AS%u (%s): %s", ctx.session.display_name.c_str(), asn,
                       ctx.args[3].c_str(), reason.c_str());
    }

    void do_unblock(CommandContext& ctx) {
        auto* asn_mgr = ctx.room.asn_reputation();
        if (!asn_mgr) {
            ctx.send_system_message("ASN reputation manager is not configured.");
            return;
        }

        uint32_t asn = 0;
        try {
            asn = static_cast<uint32_t>(std::stoul(ctx.args[2]));
        }
        catch (...) {
            ctx.send_system_message("Invalid ASN number: " + ctx.args[2]);
            return;
        }

        if (asn_mgr->unblock_asn(asn)) {
            asn_mgr->save_async("asn_reputation.json");
            ctx.send_system_message("Unblocked AS" + std::to_string(asn));
        }
        else {
            ctx.send_system_message("AS" + std::to_string(asn) + " is not blocked.");
        }
    }

    void do_list(CommandContext& ctx) {
        auto* asn_mgr = ctx.room.asn_reputation();
        if (!asn_mgr) {
            ctx.send_system_message("ASN reputation manager is not configured.");
            return;
        }

        auto flagged = asn_mgr->list_flagged();
        if (flagged.empty()) {
            ctx.send_system_message("No ASNs currently flagged.");
            return;
        }

        std::string msg = std::to_string(flagged.size()) + " flagged ASN(s):\n";
        for (auto& entry : flagged) {
            msg += "  AS" + std::to_string(entry.asn) + " (" + entry.as_org + ") — " +
                   asn_status_to_string(entry.status);
            if (!entry.block_reason.empty())
                msg += " [" + entry.block_reason + "]";
            msg += "\n";
        }

        ctx.send_system_message(msg);
    }
};

static CommandRegistrar reg(std::make_unique<AsnCommand>());
